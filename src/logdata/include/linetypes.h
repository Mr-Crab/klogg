/*
 * Copyright (C) 2016 -- 2019 Anton Filimonov and other contributors
 *
 * This file is part of klogg.
 *
 * klogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * klogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with klogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <algorithm>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <qglobal.h>
#include <string_view>

#include <QMetaType>
#include <QString>
#include <type_traits>

#include "containers.h"
#include "log.h"

template <typename S>
constexpr S maxValue()
{
    return S( std::numeric_limits<typename S::UnderlyingType>::max() );
}

// Lightweight strong-typedef base: stores the underlying value and gives
// derived types default comparison (==, !=, <, <=, >, >=) via operator<=>.
template <typename Derived, std::integral T>
class StrongType {
public:
    using UnderlyingType = T;

    constexpr StrongType() = default;
    constexpr explicit StrongType( T value )
        : value_{ value }
    {
    }

    constexpr auto operator<=>( const StrongType& ) const = default;

    constexpr T raw() const noexcept
    {
        return value_;
    }

protected:
    T value_{};
};

// Opt-in operator mixins, mirroring the exact operator set each strong type had before.
// These only go through the public raw()/constructor, since they are friends of the
// mixin itself, not of StrongType, and cannot touch its protected value_.
template <typename Derived>
struct Incrementable {
    friend Derived& operator++( Derived& d )
    {
        d = Derived( d.raw() + 1 );
        return d;
    }
    friend Derived operator++( Derived& d, int )
    {
        Derived tmp{ d };
        ++d;
        return tmp;
    }
};

template <typename Derived>
struct Decrementable {
    friend Derived& operator--( Derived& d )
    {
        d = Derived( d.raw() - 1 );
        return d;
    }
    friend Derived operator--( Derived& d, int )
    {
        Derived tmp{ d };
        --d;
        return tmp;
    }
};

template <typename Derived>
struct Addable {
    friend Derived operator+( Derived a, const Derived& b )
    {
        return Derived( a.raw() + b.raw() );
    }
    friend Derived& operator+=( Derived& a, const Derived& b )
    {
        a = Derived( a.raw() + b.raw() );
        return a;
    }
};

template <typename Derived>
struct Subtractable {
    friend Derived operator-( Derived a, const Derived& b )
    {
        return Derived( a.raw() - b.raw() );
    }
    friend Derived& operator-=( Derived& a, const Derived& b )
    {
        a = Derived( a.raw() - b.raw() );
        return a;
    }
};

// Qt file reading api has qint64 type offsets
struct OffsetInFile : StrongType<OffsetInFile, int64_t>,
                      Addable<OffsetInFile>,
                      Subtractable<OffsetInFile>,
                      Incrementable<OffsetInFile> {
    using StrongType::StrongType;

    template <typename T = UnderlyingType>
    constexpr T get() const
    {
        const auto underlyingValue = raw();

        if constexpr ( std::is_same_v<T, UnderlyingType> ) {
            return underlyingValue;
        }
        else if constexpr ( std::is_unsigned_v<T> ) {
            Q_ASSERT( underlyingValue >= 0 );
            return klogg::narrow_cast<T>( static_cast<uint64_t>( underlyingValue ) );
        }
        else {
            return klogg::narrow_cast<T>( underlyingValue );
        }
    }
};

struct LinesCount : StrongType<LinesCount, uint64_t>,
                    Addable<LinesCount>,
                    Subtractable<LinesCount>,
                    Incrementable<LinesCount>,
                    Decrementable<LinesCount> {
    using StrongType::StrongType;

    template <typename T = UnderlyingType>
    constexpr T get() const
    {
        static_assert( std::is_signed_v<T> == std::is_signed_v<UnderlyingType>,
                       "T should be the same sign as UnderlyingType" );

        if constexpr ( std::is_same_v<T, UnderlyingType> ) {
            return raw();
        }
        else {
            return klogg::narrow_cast<T>( raw() );
        }
    }
};
Q_DECLARE_METATYPE( LinesCount )

struct LineNumber : StrongType<LineNumber, uint64_t>,
                    Incrementable<LineNumber>,
                    Decrementable<LineNumber> {
    using StrongType::StrongType;

    template <typename T = UnderlyingType>
    constexpr T get() const
    {
        if constexpr ( std::is_same_v<T, UnderlyingType> ) {
            return raw();
        }
        else {
            return klogg::narrow_cast<T>( raw() );
        }
    }
};
Q_DECLARE_METATYPE( LineNumber )

struct LineLength : StrongType<LineLength, decltype( QString{}.size() )>,
                    Addable<LineLength>,
                    Subtractable<LineLength> {
    using StrongType::StrongType;

    template <typename T = UnderlyingType>
    constexpr T get() const
    {
        static_assert( std::is_signed_v<T> == std::is_signed_v<UnderlyingType>,
                       "T should be the same sign as UnderlyingType" );

        if constexpr ( std::is_same_v<T, UnderlyingType> ) {
            return raw();
        }
        else {
            return klogg::narrow_cast<T>( raw() );
        }
    }
};
Q_DECLARE_METATYPE( LineLength )

struct LineColumn : StrongType<LineColumn, decltype( QString{}.size() )>,
                    Incrementable<LineColumn> {
    using StrongType::StrongType;

    template <typename T = UnderlyingType>
    constexpr T get() const
    {
        if constexpr ( std::is_same_v<T, UnderlyingType> ) {
            return raw();
        }
        else if constexpr ( std::is_same_v<T, size_t> ) {
            Q_ASSERT( raw() >= 0 );
            return static_cast<T>( raw() );
        }
        else {
            static_assert( std::is_signed_v<T> == std::is_signed_v<UnderlyingType>,
                           "T should be the same sign as UnderlyingType" );
            return klogg::narrow_cast<T>( raw() );
        }
    }
};
Q_DECLARE_METATYPE( LineColumn )


inline constexpr OffsetInFile operator"" _offset( unsigned long long int value )
{
    return OffsetInFile( static_cast<OffsetInFile::UnderlyingType>( value ) );
}
inline constexpr LineNumber operator"" _lnum( unsigned long long int value )
{
    return LineNumber( static_cast<LineNumber::UnderlyingType>( value ) );
}
inline constexpr LinesCount operator"" _lcount( unsigned long long int value )
{
    return LinesCount( static_cast<LinesCount::UnderlyingType>( value ) );
}
inline constexpr LineLength operator"" _length( unsigned long long int value )
{
    return LineLength( static_cast<LineLength::UnderlyingType>( value ) );
}
inline constexpr LineColumn operator"" _lcol( unsigned long long int value )
{
    return LineColumn( static_cast<LineColumn::UnderlyingType>( value ) );
}

template <typename T, typename U, typename R = T>
inline R boundedAdd( const T& value, const U& inc )
{
    return ( value.get() <= maxValue<R>().get() - inc.get() ) ? R( value.get() + inc.get() )
                                                              : maxValue<R>();
}

template <typename T, typename U, typename R = T>
inline R boundedSubtract( const T& value, const U& dec )
{
    return value.get() >= dec.get() ? R( value.get() - dec.get() ) : R{};
}

inline LineNumber& operator+=( LineNumber& number, const LinesCount& count )
{
    number = boundedAdd( number, count );
    return number;
}
inline LineNumber operator+( const LineNumber& number, const LinesCount& count )
{
    return boundedAdd( number, count );
}

inline LineColumn& operator+=( LineColumn& column, const LineLength& length )
{
    column = boundedAdd( column, length );
    return column;
}
inline LineColumn operator+( const LineColumn& column, const LineLength& length )
{
    return boundedAdd( column, length );
}

inline LineNumber operator-( const LineNumber& number, const LinesCount& count )
{
    return boundedSubtract( number, count );
}

inline LineColumn operator-( const LineColumn& column, const LineLength& length )
{
    return boundedSubtract( column, length );
}

inline LinesCount operator-( const LineNumber& n1, const LineNumber& n2 )
{
    return boundedSubtract<LineNumber, LineNumber, LinesCount>( n1, n2 );
}

inline LineLength operator-( const LineColumn& n1, const LineColumn& n2 )
{
    return boundedSubtract<LineColumn, LineColumn, LineLength>( n1, n2 );
}

inline bool operator<( const LineNumber& number, const LinesCount& count )
{
    return number.get() < count.get();
}
inline bool operator>=( const LineNumber& number, const LinesCount& count )
{
    return !( number < count );
}

using OptionalLineNumber = std::optional<LineNumber>;

template <typename Derived, typename T>
QDebug operator<<( QDebug dbg, const StrongType<Derived, T>& object )
{
    QDebugStateSaver saver( dbg );
    dbg << object.raw();

    return dbg;
}

// Represents a position in a file (line, column)
class FilePosition {
public:
    FilePosition()
        : column_{ -1 }
    {
    }
    FilePosition( LineNumber line, LineColumn column )
        : line_{ line }
        , column_{ column }
    {
    }

    LineNumber line() const
    {
        return line_;
    }
    LineColumn column() const
    {
        return column_;
    }

    bool operator==( const FilePosition& other ) const
    {
        return line_ == other.line_ && column_ == other.column_;
    }

    bool operator!=( const FilePosition& other ) const
    {
        return !this->operator==( other );
    }

private:
    LineNumber line_;
    LineColumn column_;
};

// Length of a tab stop
constexpr int TabStop = 8;

inline QString untabify( QString&& line, LineColumn initialPosition = 0_lcol )
{
    line.replace( QChar::Null, QChar::Space );

    LineLength::UnderlyingType position = 0;
    position = klogg::narrow_cast<LineLength::UnderlyingType>(
        line.indexOf( QChar::Tabulation, position ) );
    while ( position >= 0 ) {
        const auto spaces = TabStop - ( ( initialPosition.get() + position ) % TabStop );
        line.replace( position, 1, QString( spaces, QChar::Space ) );
        position = klogg::narrow_cast<LineLength::UnderlyingType>(
            line.indexOf( QChar::Tabulation, position ) );
    }

    return std::move( line );
}

template <typename LineType>
LineLength getUntabifiedLength( const LineType& utf8Line )
{
    size_t totalSpaces = 0;

    auto tabPosition = utf8Line.find( '\t' );
    while ( tabPosition != std::string_view::npos ) {
        const auto spaces = TabStop - ( ( tabPosition + totalSpaces ) % TabStop ) - 1;
        totalSpaces += spaces;

        tabPosition = utf8Line.find( '\t', tabPosition + 1 );
    }

    return LineLength( klogg::narrow_cast<LineLength::UnderlyingType>(
        static_cast<int64_t>( utf8Line.size() + totalSpaces ) ) );
}
