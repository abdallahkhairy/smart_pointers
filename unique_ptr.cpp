#include <iostream>
#include <memory>  // for std::move

template<typename T>
class my_unique_ptr {
    T* rawPtr{};  // Raw pointer to the managed object

public:
    // Default constructor
    my_unique_ptr() = default;

    // Constructor with raw pointer
    explicit my_unique_ptr(T* ptr)
    : rawPtr{ptr}
    {}

    // Deleted copy constructor and copy assignment
    my_unique_ptr(const my_unique_ptr&)           = delete;
    my_unique_ptr& operator=(const my_unique_ptr&) = delete;

    // Move constructor
    my_unique_ptr(my_unique_ptr&& src) noexcept
    : rawPtr{src.rawPtr}
    {
        src.rawPtr = nullptr;
    }

    // Move assignment operator
    my_unique_ptr& operator=(my_unique_ptr&& src) noexcept
    {
        if (this != &src) {
            delete rawPtr;
            rawPtr = src.rawPtr;
            src.rawPtr = nullptr;
        }
        return *this;
    }

    // Destructor
    ~my_unique_ptr() {
        delete rawPtr;
    }

    // Dereference and -> operators
    T* operator->() { return rawPtr; }
    T& operator*() { return *rawPtr; }

    // Make unique function 
    static my_unique_ptr<T> make_unique() {
        return my_unique_ptr<T>(new T());
    }

    template<typename... Args>
    static my_unique_ptr<T> make_unique(Args&&... args) {
        return my_unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
};

// Test class for demonstration
class Test {
public:
    // Default constructor
    Test() { std::cout << "Test object created.\n"; }

    // Constructor with an integer argument
    Test(int n) { 
        std::cout << "Test object created with value: " << n << "\n"; 
    }

    ~Test() { std::cout << "Test object destroyed.\n"; }

    void say_hello() { std::cout << "Hello from Test!\n"; }
};

int main() {
    std::cout << "Creating my_unique_ptr with make_unique (no args)...\n";
    auto p1 = my_unique_ptr<Test>::make_unique(); // Calls make_unique for Test with no arguments
    std::cout << "--------------------------------------------\n";

    std::cout << "Accessing member function using operator->...\n";
    p1->say_hello(); // Test -> operator
    std::cout << "--------------------------------------------\n";

    std::cout << "\nCreating my_unique_ptr with constructor arguments...\n";
    auto p2 = my_unique_ptr<Test>::make_unique(10); 
    std::cout << "--------------------------------------------\n";

    std::cout << "\nEnd of main function. p1 and p2 will be destroyed here...\n";

    return 0;
}
