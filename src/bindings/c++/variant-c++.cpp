/*  This file is part of SAIL (https://github.com/smoked-herring/sail)

    Copyright (c) 2022 Dmitry Baryshev

    The MIT License

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include <cstring>
#include <stdexcept>
#include <string>
#include <variant>

#include "sail-c++.h"

namespace sail
{

using variant_p = std::variant<
                              bool,
                              char,
                              unsigned char,
                              short,
                              unsigned short,
                              int,
                              unsigned int,
                              long,
                              unsigned long,
                              float,
                              double,
                              std::string,
                              arbitrary_data
                              >;

class SAIL_HIDDEN variant::pimpl
{
public:
    pimpl()
        : type(SAIL_VARIANT_TYPE_INVALID)
    {
    }
    ~pimpl()
    {
    }

    variant_p value;
    SailVariantType type;
};

variant::variant()
    : d(new pimpl)
{
}

variant::variant(const sail::variant &var)
    : variant()
{
    *this = var;
}

variant& variant::operator=(const sail::variant &variant)
{
    d->type  = variant.d->type;
    d->value = variant.d->value;

    return *this;
}

variant::variant(sail::variant &&variant) noexcept
{
    *this = std::move(variant);
}

variant& variant::operator=(sail::variant &&variant) noexcept
{
    d = std::move(variant.d);

    return *this;
}

variant::~variant()
{
}

bool variant::is_valid() const
{
    return d->type != SAIL_VARIANT_TYPE_INVALID;
}

template<typename T>
bool variant::has_value() const
{
    return std::holds_alternative<T>(d->value);
}

// Allow only specific types. Other types will fail to link.
//
template SAIL_EXPORT bool variant::has_value<bool>() const;

template SAIL_EXPORT bool variant::has_value<char>() const;
template SAIL_EXPORT bool variant::has_value<unsigned char>() const;

template SAIL_EXPORT bool variant::has_value<short>() const;
template SAIL_EXPORT bool variant::has_value<unsigned short>() const;

template SAIL_EXPORT bool variant::has_value<int>() const;
template SAIL_EXPORT bool variant::has_value<unsigned int>() const;

template SAIL_EXPORT bool variant::has_value<long>() const;
template SAIL_EXPORT bool variant::has_value<unsigned long>() const;

template SAIL_EXPORT bool variant::has_value<float>() const;
template SAIL_EXPORT bool variant::has_value<double>() const;

template SAIL_EXPORT bool variant::has_value<std::string>() const;
template SAIL_EXPORT bool variant::has_value<sail::arbitrary_data>() const;

template<typename T>
const T& variant::value() const
{
    return std::get<T>(d->value);
}

// Allow only specific types. Other types will fail to link.
//
template SAIL_EXPORT const bool& variant::value<>() const;

template SAIL_EXPORT const char&          variant::value<>() const;
template SAIL_EXPORT const unsigned char& variant::value<>() const;

template SAIL_EXPORT const short&          variant::value<>() const;
template SAIL_EXPORT const unsigned short& variant::value<>() const;

template SAIL_EXPORT const int&          variant::value<>() const;
template SAIL_EXPORT const unsigned int& variant::value<>() const;

template SAIL_EXPORT const long&          variant::value<>() const;
template SAIL_EXPORT const unsigned long& variant::value<>() const;

template SAIL_EXPORT const float&  variant::value<>() const;
template SAIL_EXPORT const double& variant::value<>() const;

template SAIL_EXPORT const std::string&          variant::value<>() const;
template SAIL_EXPORT const sail::arbitrary_data& variant::value<>() const;

// Allow only specific types. Other types will fail to link.
//
template<>
SAIL_EXPORT variant& variant::with_value<>(const bool &value)
{
    d->type = SAIL_VARIANT_TYPE_BOOL;
    d->value.emplace<bool>(value);

    return *this;
}

template<>
SAIL_EXPORT variant& variant::with_value<>(const char &value)
{
    d->type = SAIL_VARIANT_TYPE_CHAR;
    d->value.emplace<char>(value);

    return *this;
}

template<>
SAIL_EXPORT variant& variant::with_value<>(const unsigned char &value)
{
    d->type = SAIL_VARIANT_TYPE_UNSIGNED_CHAR;
    d->value.emplace<unsigned char>(value);

    return *this;
}

template<>
SAIL_EXPORT variant& variant::with_value<>(const short &value)
{
    d->type = SAIL_VARIANT_TYPE_SHORT;
    d->value.emplace<short>(value);

    return *this;
}

template<>
SAIL_EXPORT variant& variant::with_value<>(const unsigned short &value)
{
    d->type = SAIL_VARIANT_TYPE_UNSIGNED_SHORT;
    d->value.emplace<unsigned short>(value);

    return *this;
}

template<>
SAIL_EXPORT variant& variant::with_value<>(const int &value)
{
    d->type = SAIL_VARIANT_TYPE_INT;
    d->value.emplace<int>(value);

    return *this;
}

template<>
SAIL_EXPORT variant& variant::with_value<>(const unsigned int &value)
{
    d->type = SAIL_VARIANT_TYPE_UNSIGNED_INT;
    d->value.emplace<unsigned int>(value);

    return *this;
}

template<>
SAIL_EXPORT variant& variant::with_value<>(const long &value)
{
    d->type = SAIL_VARIANT_TYPE_LONG;
    d->value.emplace<long>(value);

    return *this;
}

template<>
SAIL_EXPORT variant& variant::with_value<>(const unsigned long &value)
{
    d->type = SAIL_VARIANT_TYPE_UNSIGNED_LONG;
    d->value.emplace<unsigned long>(value);

    return *this;
}

template<>
SAIL_EXPORT variant& variant::with_value<>(const float &value)
{
    d->type = SAIL_VARIANT_TYPE_FLOAT;
    d->value.emplace<float>(value);

    return *this;
}

template<>
SAIL_EXPORT variant& variant::with_value<>(const double &value)
{
    d->type = SAIL_VARIANT_TYPE_DOUBLE;
    d->value.emplace<double>(value);

    return *this;
}

template<>
SAIL_EXPORT variant& variant::with_value<>(const std::string &value)
{
    d->type = SAIL_VARIANT_TYPE_STRING;
    d->value.emplace<std::string>(value);

    return *this;
}

template<>
SAIL_EXPORT variant& variant::with_value<>(const sail::arbitrary_data &value)
{
    d->type = SAIL_VARIANT_TYPE_DATA;
    d->value.emplace<sail::arbitrary_data>(value);

    return *this;
}

variant::variant(const sail_variant *variant)
    : sail::variant()
{
    if (variant == nullptr) {
        SAIL_LOG_TRACE("NULL pointer has been passed to sail::variant(). The object is untouched");
        return;
    }

    d->type = variant->type;

    switch (variant->type) {
        case SAIL_VARIANT_TYPE_BOOL:           d->value.emplace<bool>(sail_variant_to_bool(variant));                     break;
        case SAIL_VARIANT_TYPE_CHAR:           d->value.emplace<char>(sail_variant_to_char(variant));                     break;
        case SAIL_VARIANT_TYPE_UNSIGNED_CHAR:  d->value.emplace<unsigned char>(sail_variant_to_unsigned_char(variant));   break;
        case SAIL_VARIANT_TYPE_SHORT:          d->value.emplace<short>(sail_variant_to_short(variant));                   break;
        case SAIL_VARIANT_TYPE_UNSIGNED_SHORT: d->value.emplace<unsigned short>(sail_variant_to_unsigned_short(variant)); break;
        case SAIL_VARIANT_TYPE_INT:            d->value.emplace<int>(sail_variant_to_int(variant));                       break;
        case SAIL_VARIANT_TYPE_UNSIGNED_INT:   d->value.emplace<unsigned int>(sail_variant_to_unsigned_int(variant));     break;
        case SAIL_VARIANT_TYPE_LONG:           d->value.emplace<long>(sail_variant_to_long(variant));                     break;
        case SAIL_VARIANT_TYPE_UNSIGNED_LONG:  d->value.emplace<unsigned long>(sail_variant_to_unsigned_long(variant));   break;
        case SAIL_VARIANT_TYPE_FLOAT:          d->value.emplace<float>(sail_variant_to_float(variant));                   break;
        case SAIL_VARIANT_TYPE_DOUBLE:         d->value.emplace<double>(sail_variant_to_double(variant));                 break;
        case SAIL_VARIANT_TYPE_STRING:         d->value.emplace<std::string>(sail_variant_to_string(variant));            break;
        case SAIL_VARIANT_TYPE_DATA: {
            const void *data = sail_variant_to_data(variant);
            sail::arbitrary_data arbitrary_data(variant->size);
            memcpy(arbitrary_data.data(), data, variant->size);
            d->value.emplace<sail::arbitrary_data>(arbitrary_data);
            break;
        }
        case SAIL_VARIANT_TYPE_INVALID: break;
    }
}

sail_status_t variant::to_sail_variant(sail_variant **variant) const
{
    SAIL_CHECK_PTR(variant);

    struct sail_variant *variant_local;
    SAIL_TRY(sail_alloc_variant(&variant_local));

    SAIL_AT_SCOPE_EXIT(
        sail_destroy_variant(variant_local);
    );

    variant_local->type = d->type;

    switch (d->type) {
        case SAIL_VARIANT_TYPE_BOOL:           sail_set_variant_bool(variant_local,           std::get<bool>(d->value));                break;
        case SAIL_VARIANT_TYPE_CHAR:           sail_set_variant_char(variant_local,           std::get<char>(d->value));                break;
        case SAIL_VARIANT_TYPE_UNSIGNED_CHAR:  sail_set_variant_unsigned_char(variant_local,  std::get<unsigned char>(d->value));       break;
        case SAIL_VARIANT_TYPE_SHORT:          sail_set_variant_short(variant_local,          std::get<short>(d->value));               break;
        case SAIL_VARIANT_TYPE_UNSIGNED_SHORT: sail_set_variant_unsigned_short(variant_local, std::get<unsigned short>(d->value));      break;
        case SAIL_VARIANT_TYPE_INT:            sail_set_variant_int(variant_local,            std::get<int>(d->value));                 break;
        case SAIL_VARIANT_TYPE_UNSIGNED_INT:   sail_set_variant_unsigned_int(variant_local,   std::get<unsigned int>(d->value));        break;
        case SAIL_VARIANT_TYPE_LONG:           sail_set_variant_long(variant_local,           std::get<long>(d->value));                break;
        case SAIL_VARIANT_TYPE_UNSIGNED_LONG:  sail_set_variant_unsigned_long(variant_local,  std::get<unsigned long>(d->value));       break;
        case SAIL_VARIANT_TYPE_FLOAT:          sail_set_variant_float(variant_local,          std::get<float>(d->value));               break;
        case SAIL_VARIANT_TYPE_DOUBLE:         sail_set_variant_double(variant_local,         std::get<double>(d->value));              break;
        case SAIL_VARIANT_TYPE_STRING:         sail_set_variant_string(variant_local,         std::get<std::string>(d->value).c_str()); break;
        case SAIL_VARIANT_TYPE_DATA: {
            const sail::arbitrary_data &arbitrary_data = std::get<sail::arbitrary_data>(d->value);
            sail_set_variant_data(variant_local, arbitrary_data.data(), arbitrary_data.size());
            break;
        }
        case SAIL_VARIANT_TYPE_INVALID: break;
    }

    *variant = variant_local;
    variant_local = nullptr;

    return SAIL_OK;
}

}
