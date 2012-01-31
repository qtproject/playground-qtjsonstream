#ifndef VARIANT_H
#define VARIANT_H

#include <map>
#include <vector>
#include <string>

class SmartPointerBase;

class VariantList;
class VariantMap;

class Variant
{
public:
    Variant() : _type(Invalid) {}
    Variant(bool v) : _type(Bool) { _bVal = v; }
    Variant(int v) : _type(Int) { _nVal = v; }
    Variant(double v)  : _type(Double) { _dVal = v; }
    Variant(long long v)  : _type(LongLong) { _llVal = v; }
    Variant(const char * v);

    Variant(const std::string &v);
    Variant(const VariantList &v);
    Variant(const VariantMap &v);

    Variant(const Variant &v);
    ~Variant();

    Variant& operator = (const Variant& );

    enum Type
    {
        Invalid = -1, Bool, Int, LongLong, Double, String, Map, List
    };

    Type type() const { return _type; }
    bool isValid() const { return Invalid != _type; }

    bool toBool() const { return type() == Bool ? _bVal : false; }
    int toInt() const { return type() == Int ? _nVal : 0; }
    int toLongLong() const { return type() == LongLong ? _llVal : 0; }
    double toDouble() const { return type() == Double ? _dVal : 0; }
    std::string toString() const;
    VariantList toList() const;
    VariantMap toMap() const;

private:

    Type _type;

    union
    {
        bool _bVal;
        int _nVal;
        double _dVal;
        long long _llVal;
        SmartPointerBase *_ptr;
    };
};

class VariantMap : public std::map<std::string, Variant>
{
public:
    void insert(const std::string & key, const Variant & val)
    {
        std::map<std::string, Variant>::insert(std::pair<std::string, Variant>(key,val));
    }

    bool contains(const std::string & val) const
    {
        return find(val) != end();
    }

    Variant value(const std::string & val) const
    {
        std::map<std::string, Variant>::const_iterator it;
        it = find(val);
        return it != end() ? it->second : Variant();
    }
};

class VariantList : public std::vector<Variant>
{
public:
    void append(const Variant & val)
    {
        push_back(val);
    }

    void insert(int nPos, const Variant & val)
    {
        std::vector<Variant>::insert(begin() + nPos, val);
    }

    template<typename T>
    inline VariantList &operator<<(const T & val)
    {
        append(val);

        return *this;
    }
};

#endif // VARIANT_H
