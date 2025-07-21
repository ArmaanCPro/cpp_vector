#include <iostream>
#include <cstdint>
#include <memory>
#include <cassert>

template <typename T, class Alloc = std::allocator<T>>
class Vec
{
private:
    class iterator
    {
    public:
        iterator(T* ptr)
        : m_ptr(ptr)
        {}

        T& operator*() { return *m_ptr; }
        T* operator->() { return m_ptr; }

        iterator& operator++() { return m_ptr++; }
        iterator operator++(int)
        {
            iterator temp = *this;
            m_ptr++;
            return temp;
        }

        bool operator==(const iterator& other) const
        {
            return m_ptr == other.m_ptr;
        }

        bool operator!=(const iterator& other) const
        {
            return !(*this == other);
        }
    private:
        T* m_ptr = nullptr;
    };

public:
    Vec() = default;

    Vec(uint32_t num_elements)
        :
        size(num_elements)
    {
       resize(size);
    }

    Vec(std::initializer_list<T> list)
        :
        size(list.size())
    {
        resize(size);

        int index = 0;
        for (auto it = list.begin(); it != list.end(); it++)
        {
            data[index++] = *it;
        }
    }

    Vec(Vec<T, Alloc>&& other) noexcept
    {
        data = other.data;
        size = other.size;
        capacity = other.capacity;
    }

    Vec& operator=(const Vec<T, Alloc>& other) noexcept
    {
        if (*this == other)
            return *this;

        auto allocator = Alloc{};

        if (data)
        {
            // could have a smarter way, instead of deallocating we could use previously allocated memory if it exists and expand/decrease as needed
            for (int i = 0; i < size; i++)
            {
                 std::allocator_traits<Alloc>::destroy(allocator, &data[i]);
            }
            std::allocator_traits<Alloc>::deallocate(allocator, data, capacity);
        }
        std::allocator_traits<Alloc>::allocate(allocator, data, other.capacity);
        size = other.size;
        capacity = other.capacity;

        for (int i = 0; i < size; i++)
        {
            data[i] = other[i];
        }
        return *this;
    }

    ~Vec()
    {
        auto allocator = Alloc{};
        for (int i = 0; i < size; i++)
        {
            std::allocator_traits<Alloc>::destroy(allocator, &data[i]);
        }
        std::allocator_traits<Alloc>::deallocate(allocator, data, capacity);
    }

    void resize(uint32_t new_size)
    {
        auto allocator = Alloc{};

        uint32_t old_size = new_size;
        size = new_size;
        if (size >= capacity)
        {
            try_expand();
        }

        if (old_size > new_size)
        {
            // truncate
            for (int i = old_size; i > new_size; i--)
            {
                std::allocator_traits<Alloc>::destroy(allocator, &data[i]);
            }
        }
        else
        {
            // construct some new objects
            for (int i = old_size; i < new_size; i++)
            {
                std::allocator_traits<Alloc>::construct(allocator, &data[i]);
            }
        }
    }

    void reserve(uint32_t amount)
    {
        if (amount <= capacity)
        {
            return;
        }

        capacity = amount;
        try_expand();
    }

    void push_back(T element)
    {
        resize(size + 1);
        data[size - 1] = element;
    }

    T pop_back()
    {
        auto allocator = Alloc{};
        T value = data[size - 1];
        std::allocator_traits<Alloc>::destroy(allocator, &data[size - 1]);
        resize(size - 1);
        return value;
    }

    iterator begin() // this and end() should be const, but because we don't have a const iterator, these have to be non-const
    {
        return iterator{data};
    }
    iterator end()
    {
        return iterator{&data[size]};
    }

    T* get_data()
    {
        return data;
    }

    T& operator[](int index)
    {
        assert(index < size); // sanity check, std::vector has undefined behavior
        return data[index];
    }
private:

    void try_expand()
    {
        auto allocator = Alloc{};

        // could use allocate_at_least but my g++ does not support latest c++23 :(
        auto new_allocation = std::allocator_traits<Alloc>::allocate(allocator, size > capacity ? size * 2 : capacity * 2);

        auto old_capacity = capacity;
        capacity = size > capacity ? size * 2 : capacity * 2;

        if (data)
        {
            for (int i = 0; i < size; i++)
            {
                if (std::is_nothrow_move_constructible<T>::value)
                    new_allocation[i] = std::move(data[i]);
                else
                    new_allocation[i] = data[i];
            }
            allocator.deallocate(data, old_capacity);
        }
        data = new_allocation;
    }
    T* data = nullptr;
    uint32_t size = 0;
    uint32_t capacity = 0;
};

int main()
{
    Vec<int> a = { 1, 4, 3};
    a.push_back(5);

    for (auto i = a.begin(); i != a.end(); i++)
    {
         std::cout << *i << std::endl;
    }
}
