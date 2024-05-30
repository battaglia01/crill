# crill::reclaim_on_write_object Documentation

**NOTE**: as of this writing, this class was still in development and had not been tested. The documentation is based on the design and intended implementation, but there may still be some race conditions involved. **DON'T USE IT YET!**

## Overview
The `crill::reclaim_on_write_object` is a template class that, like `crill::reclaim_object`, provides concurrent read and write access to an object of type `T`. However, it differs significantly in its approach to reclamation. Instead of maintaining a zombie list and requiring manual reclamation, it **automatically handles reclamation on update**, simplifying the usage and reducing overhead.

## Key Differences from `crill::reclaim_object`
- **Automatic Reclamation**: There is no need to call `reclaim()`. The reclamation occurs automatically during the `update` process.
- **No Zombie List**: It does not maintain a zombie list, avoiding the need for heap allocation for overwritten objects.
- **Blocking on Update**: Writers will block during the `update` until all readers have finished accessing the old value.

The advantage of this algorithm is that it does not need to call `reclaim()`, as the reclamation happens automatically on update. The disadvantage is that the writer needs to block until all readers have finished, which can introduce latency and contention.

The class uses a pair of slots to store the current and previous values of the object. When a writer updates the value, it swaps the slots and waits for all readers accessing the old value to finish. This way, the old value can be safely overwritten without affecting the readers.

## Usage

### Creating a reclaim_on_write_object
To create a `reclaim_on_write_object`, you can either use the default constructor or pass constructor arguments for the value type `T`:

```cpp
crill::reclaim_on_write_object<int> myObject; // Default-constructed value
crill::reclaim_on_write_object<std::string> myString("Hello"); // Value constructed with arguments
```

### Reading from reclaim_on_write_object
Reading from a `reclaim_on_write_object` is the same as reading from a `reclaim_object`. You need to use a `reader` object and a `read_ptr` object:

```cpp
auto reader = myObject.get_reader(); // Create a reader
auto read_ptr = reader.read_lock(); // Create a read_ptr
int value = *read_ptr; // Access the value
// ... do something with value ...
// The read_ptr is destroyed here, ending the read access
```

### Writing to reclaim_on_write_object
Writing to a `reclaim_on_write_object` is also similar to writing to a `reclaim_object`. You can use the `update` method or a `write_ptr` object:

```cpp
myObject.update(42); // Update the value with a new one

auto write_ptr = myObject.write_lock(); // Create a write_ptr
*write_ptr = 42; // Modify the value
// ... do other modifications ...
// The write_ptr is destroyed here, publishing the new value
```

Note that both `update` and the destructor of `write_ptr` will block until all readers accessing the old value have finished.

### Reclamation
Unlike `reclaim_object`, there is no need to manually call a `reclaim` function. Reclamation happens automatically when the value is updated.

# Public Interface

## Class: `reclaim_on_write_object<T>`
**Purpose**: Manages concurrent access to an object of type `T`, allowing for wait-free reading and safe writing with automatic reclamation.

- `reclaim_on_write_object()`
  - *Constructor*: Initializes a `reclaim_on_write_object` with a default-constructed value of type `T`.
- `reclaim_on_write_object(Args&&... args)`
  - *Constructor*: Initializes a `reclaim_on_write_object` with a value constructed with the provided constructor arguments.
- `get_reader()`
  - *Function*: Returns a `reader` object associated with the `reclaim_on_write_object`.
- `update(Args&&... args)`
  - *Function*: Updates the current value to a new value constructed from the provided constructor arguments. Blocks until all readers accessing the old value have finished.
- `write_lock()`
  - *Function*: Returns a `write_ptr` giving scoped write access to the current value.

## Class: `reclaim_on_write_object<T>::reader`
**Purpose**: Registers with the `reclaim_on_write_object` to safely read the current value without blocking writers.

- `reader(reclaim_on_write_object& obj)`
  - *Constructor*: Constructs a `reader` that registers itself with the given `reclaim_on_write_object`.
- `~reader()`
  - *Destructor*: Unregisters the `reader` from the `reclaim_on_write_object`.
- `get_value()`
  - *Function*: Returns a copy of the current value.
- `read_lock()`
  - *Function*: Returns a `read_ptr` giving read access to the current value.

## Class: `reclaim_on_write_object<T>::read_ptr`
**Purpose**: Provides scoped read access to the value within the `reclaim_on_write_object`, ensuring a consistent view of the value during its scope.

- `read_ptr(reader& rdr)`
  - *Constructor*: Constructs a `read_ptr` that provides scoped read access to the value.
- `~read_ptr()`
  - *Destructor*: Ends the scoped read access.
- `operator*()`
  - *Function*: Dereferences the `read_ptr` to access the value.
- `operator->()`
  - *Function*: Provides pointer access to the value.

## Class: `reclaim_on_write_object<T>::write_ptr`
**Purpose**: Offers scoped write access to the value within the `reclaim_on_write_object`, allowing modifications to be made and published atomically.

- `write_ptr(reclaim_on_write_object& obj)`
  - *Constructor*: Constructs a `write_ptr` that provides scoped write access to the value.
- `~write_ptr()`
  - *Destructor*: Publishes the new value atomically when the `write_ptr` goes out of scope. Blocks until all readers accessing the old value have finished.
- `operator*()`
  - *Function*: Dereferences the `write_ptr` to access or modify the value.
- `operator->()`
  - *Function*: Provides pointer access to the modifiable value.


# Thread Safety and Performance Considerations

The `crill::reclaim_on_write_object` is designed to be thread-safe, allowing multiple threads to read and write concurrently without the need for external synchronization. The wait-free guarantee for readers ensures that the realtime thread (audio thread) can access the object without delay, which is critical for audio processing.

Writers may block other writers and will also block on update until all readers accessing the old value have finished. This means that write operations should be used judiciously to avoid contention.

The automatic reclamation on write eliminates the need for a separate reclamation process, which can simplify usage. However, it also means that writers may experience more blocking compared to `reclaim_object`.

# Best Practices

- **Read Access**: Use `read_ptr` within the smallest possible scope to ensure that the value is accessed only when needed and to minimize the time that writers may be blocked.
- **Write Access**: Use `write_ptr` when you need to make multiple modifications to ensure atomicity. For single modifications, `update` can be more efficient.
- **Copyable Types**: `reclaim_on_write_object` only works for types that are copy constructible and copy assignable. If your type does not meet these requirements, consider using `reclaim_object` instead.

# Limitations

- **Writer Blocking**: The `update` method and the destructor of `write_ptr` block until all readers accessing the old value have finished, which can lead to performance bottlenecks if not managed properly.
- **Non-Copyable/Movable Types**: `reclaim_on_write_object` does not work for types that are not copy constructible and copy assignable.
- **Non-Copyable/Movable Object**: To maintain thread safety, `reclaim_on_write_object` instances cannot be copied or moved. This design choice ensures that the internal state related to thread safety is not inadvertently compromised.

# Notes

- The `reclaim_on_write_object` uses a double-buffering technique with two slots to hold the current and next value. This allows readers to always access a consistent value, even while a write is in progress.
- The epoch counter is used to track which readers are accessing which version of the value. When a write happens, the epoch is incremented, and the writer waits for all readers on the old epoch to finish before proceeding.
- The algorithm requires a 64-bit lock-free atomic counter for the epoch to avoid overflow. This is checked with a static assertion.
- There is a TODO comment indicating that the algorithm may need to swap the slot and wait for readers twice before writing again to avoid a race condition, as mentioned by Paul McKenney. This should be investigated and addressed if necessary.
- Progressive backoff could be implemented in the writer when waiting for readers to improve performance under contention.