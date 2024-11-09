#include <iostream>
#include <atomic>
#include <memory>
#include <new>  // for placement new

template <typename T>
class my_shared_ptr; // Forward declaration for friendship

template <typename T>
class my_weak_ptr; // Forward declaration for weak_ptr

template <typename T>
class control_block {
public:
    std::atomic<int> strong_count;
    std::atomic<int> weak_count;

    control_block() : strong_count(1), weak_count(0) {}

    void add_weak() {
        weak_count.fetch_add(1, std::memory_order_relaxed);
    }

    void release_weak() {
        if (weak_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete this;
        }
    }

    void release_strong() {
        if (strong_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete static_cast<T*>(this + 1); // Delete the object itself
            release_weak();
        }
    }
};

template <typename T>
class my_shared_ptr {
    T* ptr{};
    control_block<T>* control{};

    my_shared_ptr(T* p, control_block<T>* cb) : ptr(p), control(cb) {}

public:
    friend class my_weak_ptr<T>; // Grant access to my_weak_ptr

    my_shared_ptr() : ptr(nullptr), control(nullptr) {}

    explicit my_shared_ptr(T* p) : ptr(p), control(new control_block<T>()) {}

    my_shared_ptr(const my_shared_ptr& other) noexcept : ptr(other.ptr), control(other.control) {
        if (control) {
            control->strong_count.fetch_add(1, std::memory_order_relaxed);
        }
    }

    my_shared_ptr(my_shared_ptr&& other) noexcept : ptr(other.ptr), control(other.control) {
        other.ptr = nullptr;
        other.control = nullptr;
    }

    my_shared_ptr& operator=(const my_shared_ptr& other) noexcept {
        if (this != &other) {
            release();
            ptr = other.ptr;
            control = other.control;
            if (control) {
                control->strong_count.fetch_add(1, std::memory_order_relaxed);
            }
        }
        return *this;
    }

    my_shared_ptr& operator=(my_shared_ptr&& other) noexcept {
        if (this != &other) {
            release();
            ptr = other.ptr;
            control = other.control;
            other.ptr = nullptr;
            other.control = nullptr;
        }
        return *this;
    }

    ~my_shared_ptr() {
        release();
    }

    T& operator*() const { return *ptr; }
    T* operator->() const { return ptr; }

    int use_count() const {
        return control ? control->strong_count.load(std::memory_order_acquire) : 0;
    }

    template <typename... Args>
    static my_shared_ptr<T> make_shared(Args&&... args) {
        void* raw_memory = ::operator new(sizeof(control_block<T>) + sizeof(T));
        control_block<T>* cb = new (raw_memory) control_block<T>();
        T* obj = new (cb + 1) T(std::forward<Args>(args)...);
        return my_shared_ptr<T>(obj, cb);
    }

private:
    void release() {
        if (control && control->strong_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            ptr->~T();
            control->release_weak();
        }
    }
};

template <typename T>
class my_weak_ptr {
    T* ptr{};
    control_block<T>* control{};

public:
    my_weak_ptr() : ptr(nullptr), control(nullptr) {}

    my_weak_ptr(const my_shared_ptr<T>& shared) : ptr(shared.ptr), control(shared.control) {
        if (control) {
            control->add_weak();
        }
    }

    my_weak_ptr(const my_weak_ptr& other) : ptr(other.ptr), control(other.control) {
        if (control) {
            control->add_weak();
        }
    }

    my_weak_ptr& operator=(const my_shared_ptr<T>& shared) {
        reset();
        ptr = shared.ptr;
        control = shared.control;
        if (control) {
            control->add_weak();
        }
        return *this;
    }

    ~my_weak_ptr() {
        reset();
    }

    int use_count() const {
        return control ? control->strong_count.load(std::memory_order_acquire) : 0;
    }

    bool expired() const {
        return use_count() == 0;
    }

    my_shared_ptr<T> lock() const {
        if (!expired()) {
            return my_shared_ptr<T>(*this);
        }
        return my_shared_ptr<T>();
    }

private:
    void reset() {
        if (control) {
            control->release_weak();
        }
        ptr = nullptr;
        control = nullptr;
    }
};

// Test class
class Test {
public:
    Test() { std::cout << "Test object created.\n"; }
    Test(int n) { std::cout << "Test object created with value: " << n << "\n"; }
    ~Test() { std::cout << "Test object destroyed.\n"; }

    void say_hello() { std::cout << "Hello from Test!\n"; }
};

int main() {
    std::cout << "Creating my_shared_ptr with make_shared (no args)...\n";
    auto sp1 = my_shared_ptr<Test>::make_shared(); // Calls make_shared for Test with no arguments
    std::cout << "--------------------------------------------\n";

    std::cout << "Accessing member function using operator->...\n";
    sp1->say_hello(); // Calls say_hello on the Test object
    std::cout << "--------------------------------------------\n";

    std::cout << "\nCreating my_shared_ptr with constructor arguments...\n";
    auto sp2 = my_shared_ptr<Test>::make_shared(10);  // Calls make_shared for Test with an integer argument
    std::cout << "--------------------------------------------\n";

    std::cout << "Creating my_weak_ptr from shared_ptr...\n";
    my_weak_ptr<Test> wp1 = sp1;
    std::cout << "Weak pointer use count: " << wp1.use_count() << "\n";
    std::cout << "--------------------------------------------\n";

    std::cout << "\nEnd of main function. Shared and weak pointers will be destroyed...\n";
    return 0;
}
