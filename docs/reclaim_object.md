# crill::reclaim_object Documentation

## Overview
The `crill::reclaim_object` is a template class that provides concurrent read and write access to an object of type `T`. It implements the **RCU pattern**, which means that readers are always wait-free and writers may block other writers but not readers.

The class is designed for audio applications, where there is typically one realtime thread (the audio thread) that only reads, and several non-realtime threads that may write.

The class maintains a "zombie list" of overwritten objects, which can be reclaimed by calling `reclaim()`. Reclamation is not automatic, but must be done manually by the user, preferably on a timer. Reclamation blocks writers, so it should be done on a non-realtime thread.

## Usage

### Creating a reclaim_object
To create a `reclaim_object`, you can either use the default constructor or pass constructor arguments for the value type `T`:

```cpp
crill::reclaim_object<int> myObject; // Default-constructed value
crill::reclaim_object<std::string> myString("Hello"); // Value constructed with arguments
```

### Reading from reclaim_object
To read from a `reclaim_object`, you need to use a `reader` object, which registers itself with the `reclaim_object` and helps track which versions are still in use. Then, you need to use a `read_ptr` object, which provides scoped access to the current value. For example:

```cpp
auto reader = myObject.get_reader(); // Create a reader
auto read_ptr = reader.read_lock(); // Create a read_ptr
int value = *read_ptr; // Access the value
// ... do something with value ...
// The read_ptr is destroyed here, ending the read access
```

Note that you need both a `reader` and a `read_ptr` to read from the object. The `reader` must outlive the `read_ptr`. Also, you can only have one `read_ptr` active at a time for any particular `reader`.

### Writing to reclaim_object
To write to a `reclaim_object`, you have two options: use the `update` method or use a `write_ptr` object. The `update` method replaces the current value with a new one constructed from the given arguments. The `write_ptr` object allows you to modify a copy of the current value, which is then published when the `write_ptr` goes out of scope. For example:

```cpp
myObject.update(42); // Update the value with a new one

auto write_ptr = myObject.write_lock(); // Create a write_ptr
*write_ptr = 42; // Modify the value
// ... do other modifications ...
// The write_ptr is destroyed here, publishing the new value
```

Note that the `write_ptr` is not required to write to the object, but it can be useful if you want to make multiple changes or modify only part of a larger object.

### Reclaiming Unused Objects
To reclaim unused objects from the zombie list, you need to call the `reclaim` method:

```cpp
myObject.reclaim(); // Reclaim unused objects
```

Note that this method blocks writers, so it should be called from a non-realtime thread and not too frequently.

# Public Interface

## Class: `reclaim_object<T>`
**Purpose**: Manages concurrent access to an object of type `T`, allowing for wait-free reading and safe writing.

- `reclaim_object()`
  - *Constructor*: Initializes a `reclaim_object` with a default-constructed value of type `T`.
- `reclaim_object(Args... args)`
  - *Constructor*: Initializes a `reclaim_object` with a value constructed with the provided constructor arguments.
- `get_reader()`
  - *Function*: Returns a `reader` object associated with the `reclaim_object`.
- `update(Args... args)`
  - *Function*: Updates the current value to a new value constructed from the provided constructor arguments.
- `write_lock()`
  - *Function*: Returns a `write_ptr` giving scoped write access to the current value.
- `reclaim()`
  - *Function*: Deletes all previously overwritten values that are no longer referred to by a `read_ptr`.

## Class: `reclaim_object<T>::reader`
**Purpose**: Registers with the `reclaim_object` to safely read the current value without blocking writers.

- `reader(reclaim_object& obj)`
  - *Constructor*: Constructs a `reader` that registers itself with the given `reclaim_object`.
- `~reader()`
  - *Destructor*: Unregisters the `reader` from the `reclaim_object`.
- `get_value()`
  - *Function*: Returns a copy of the current value.
- `read_lock()`
  - *Function*: Returns a `read_ptr` giving read access to the current value.

## Class: `reclaim_object<T>::read_ptr`
**Purpose**: Provides scoped read access to the value within the `reclaim_object`, ensuring a consistent view of the value during its scope.

- `read_ptr(reader& rdr)`
  - *Constructor*: Constructs a `read_ptr` that provides scoped read access to the value.
- `~read_ptr()`
  - *Destructor*: Ends the scoped read access.
- `operator*()`
  - *Function*: Dereferences the `read_ptr` to access the value.
- `operator->()`
  - *Function*: Provides pointer access to the value.

## Class: `reclaim_object<T>::write_ptr`
**Purpose**: Offers scoped write access to the value within the `reclaim_object`, allowing modifications to be made and published atomically.

- `write_ptr(reclaim_object& obj)`
  - *Constructor*: Constructs a `write_ptr` that provides scoped write access to the value.
- `~write_ptr()`
  - *Destructor*: Publishes the new value atomically when the `write_ptr` goes out of scope.
- `operator*()`
  - *Function*: Dereferences the `write_ptr` to access or modify the value.
- `operator->()`
  - *Function*: Provides pointer access to the modifiable value.


# Thread Safety and Performance Considerations

The `crill::reclaim_object` is designed to be thread-safe, allowing multiple threads to read and write concurrently without the need for external synchronization. The wait-free guarantee for readers ensures that the realtime thread (audio thread) can access the object without delay, which is critical for audio processing.

Writers may block other writers, but not readers, which means that write operations should be used judiciously to avoid contention. The use of a "zombie list" allows for the safe reclamation of old values without impacting the readers.

The manual reclamation process is a trade-off between automatic garbage collection and performance. By requiring the user to call `reclaim()` periodically, it avoids the overhead of automatic reclamation mechanisms, which can be unpredictable and may introduce latency, especially in a realtime context.

# Best Practices

- **Read Access**: Use `read_ptr` within the smallest possible scope to ensure that the value is accessed only when needed.
- **Write Access**: Use `write_ptr` when you need to make multiple modifications to ensure atomicity. For single modifications, `update` can be more efficient.
- **Reclamation**: Call `reclaim()` from a non-realtime thread, and only as often as necessary to clean up the zombie list without causing undue contention.

# Limitations

- **Reclamation Blocking**: The `reclaim()` method blocks writers, which can lead to performance bottlenecks if not managed properly.
- **Non-Copyable/Movable**: To maintain thread safety, `reclaim_object` instances cannot be copied or moved. This design choice ensures that the internal state related to thread safety is not inadvertently compromised.
