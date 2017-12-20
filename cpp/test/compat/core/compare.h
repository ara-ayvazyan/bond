#pragma once

#include <algorithm>
#include <complex>
#include <limits>


template <std::size_t I = 0, typename T>
typename boost::enable_if_c<(I == T::Schema::field_count::value), bool>::type
CompareFields(const T& /*left*/, const T& /*right*/)
{
    return true;
}

template <std::size_t I = 0, typename T>
typename boost::disable_if_c<(I == T::Schema::field_count::value), bool>::type
CompareFields(const T& left, const T& right);


inline bool Equal(double left, double right)
{
    const int ulp = 5;
    return std::abs(left - right) <= std::numeric_limits<double>::epsilon() * (std::max)(std::abs(left), std::abs(right)) * ulp;
}

template <typename T>
typename boost::enable_if<bond::is_basic_type<T>, bool>::type
Equal(const T& left, const T& right)
{
    return left == right;
}


inline bool Equal(const bond::blob& left, const bond::blob& right)
{
    return left == right;
}


template <typename T>
typename boost::enable_if<bond::has_schema<T>, bool>::type
Equal(const T& left, const T& right)
{
    return CompareFields(left, right);
}


template <typename T>
inline bool Equal(const bond::bonded<T>& left, const bond::bonded<T>& right)
{
    T left_value, right_value;

    left.Deserialize(left_value);
    right.Deserialize(right_value);

    return Equal(left_value, right_value);
}


template <typename T1, typename T2>
bool Equal(const std::pair<T1, T2>& left, const std::pair<T1, T2>& right);


template <typename T>
typename boost::enable_if<bond::is_container<T>, bool>::type
Equal(const T& left, const T& right)
{
    bond::const_enumerator<T> left_items(left), right_items(right);

    while (left_items.more() && right_items.more())
        if (!Equal(left_items.next(), right_items.next()))
            return false;

    return !left_items.more() && !right_items.more();
}


template <typename T1, typename T2>
bool Equal(const std::pair<T1, T2>& left, const std::pair<T1, T2>& right)
{
    return Equal(left.first, right.first)
        && Equal(left.second, right.second);
}


template <std::size_t I, typename T>
typename boost::disable_if_c<(I == T::Schema::field_count::value), bool>::type
CompareFields(const T& left, const T& right)
{
    using Field = bond::detail::field_info<T, I>;

    return Equal(Field::GetVariable(left), Field::GetVariable(right))
        && CompareFields<I + 1>(left, right);
}
