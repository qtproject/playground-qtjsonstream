#include "variant.h"

#include <string.h>

/*!
    \class RefCounter
    \brief The RefCounter class implements a simple reference counter.
*/
class RefCounter
{
private:
    int count; // Reference count

    public:
    void AddRef()
    {
        // Increment the reference count
        count++;
    }

    int Release()
    {
        // Decrement the reference count and return the reference count.
        return --count;
    }
};

/*!
    \class SmartPointerBase
    \brief The SmartPointerBase class is a base class for SmartPointer.
*/
class SmartPointerBase
{
public:
    virtual ~SmartPointerBase() {}
    virtual SmartPointerBase* create() = 0;

protected:
    RefCounter* reference; // Reference count
};

/*!
    \class SmartPointer
    \brief The SmartPointer class implements a simple smart pointer template.

    Note: The SmartPointer is not thread safe.
*/
template < typename T > class SmartPointer : public SmartPointerBase
{
private:
    T*    pData;       // pointer

public:
    SmartPointer() : pData(0)
    {
        // Create a new reference and increment the reference count
        reference = new RefCounter();
        reference->AddRef();
    }

    SmartPointer(T* pValue) : pData(pValue)
    {
        // Create a new reference and increment the reference count
        reference = new RefCounter();
        reference->AddRef();
    }

    // Copy constructor
    // Copy the data and reference pointer and increment the reference count
    SmartPointer(const SmartPointer<T>& sp) : pData(sp.pData)
    {
        reference = sp.reference;
        reference->AddRef();
    }

    // Destructor
    // Decrement the reference count if reference become zero delete the data
    ~SmartPointer()
    {
        if (reference->Release() == 0)
        {
            delete pData;
            delete reference;
        }
    }

    T& operator* ()
    {
        return *pData;
    }

    T* operator-> ()
    {
        return pData;
    }

    SmartPointer<T>& operator = (const SmartPointer<T>& sp)
    {
        // Assignment operator
        if (this != &sp) // Avoid self assignment
        {
            // Decrement the old reference count if reference become zero delete the old data
            if (reference->Release() == 0)
            {
                delete pData;
                delete reference;
            }

            // Copy the data and reference pointer and increment the reference count
            pData = sp.pData;
            reference = sp.reference;
            reference->AddRef();
        }
        return *this;
    }

    SmartPointerBase* create()
    {
        return new SmartPointer<T>(*this);
    }
};

/*!
    \class Variant
    \brief The Variant class is a variant type implementation
*/
Variant::Variant(const char * v)
    : _type(String)
{
    _ptr = new SmartPointer<std::string>(new std::string(v));
}

Variant::Variant(const std::string &v)
    : _type(String)
{
    _ptr = new SmartPointer<std::string>(new std::string(v));
}

Variant::Variant(const VariantList &v)
    : _type(List)
{
    _ptr = new SmartPointer<VariantList>(new VariantList(v));
}

Variant::Variant(const VariantMap &v)
    : _type(Map)
{
    _ptr = new SmartPointer<VariantMap>(new VariantMap(v));
}

// Copy constructor
Variant::Variant(const Variant &v)
{
    memcpy(this, &v, sizeof(Variant));
    if (type() >= String)
    {
        _ptr = v._ptr->create();
    }
}

// Assignment operator
Variant& Variant::operator = (const Variant &v)
{
    if (this != &v) // Avoid self assignment
    {
        if (type() >= String)
        {
            delete _ptr;
        }

        memcpy(this, &v, sizeof(Variant));
        if (type() >= String)
        {
            _ptr = v._ptr->create();
        }
    }
    return *this;
}

Variant::~Variant()
{
    if (type() >= String)
    {
        delete _ptr;
    }
}

std::string Variant::toString() const
{
    return type() == String ? **static_cast< SmartPointer<std::string> *>(_ptr) : "";
}

/*!
    \class VariantList
    \brief The VariantList class is a variant list implementation
*/
VariantList Variant::toList() const
{
    return type() == List ? **static_cast< SmartPointer<VariantList> *>(_ptr) : VariantList();
}

/*!
    \class VariantMap
    \brief The VariantMap class is a variant map implementation
*/
VariantMap Variant::toMap() const
{
    return type() == Map ? **static_cast< SmartPointer<VariantMap> *>(_ptr) : VariantMap();
}
