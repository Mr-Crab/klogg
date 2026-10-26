/*
 * Copyright (C) 2026 Xiaohuang Zhu and other contributors
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

#ifndef KLOGG_LINECOLORLABELS_H
#define KLOGG_LINECOLORLABELS_H

#include "containers.h"
#include "linetypes.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>

class LineColorLabels {
public:
    void set( size_t label, const klogg::vector<LineNumber>& lines )
    {
        for ( const auto line : lines ) {
            labels_[ line.get() ] = label;
        }
    }

    void remove( const klogg::vector<LineNumber>& lines )
    {
        for ( const auto line : lines ) {
            labels_.erase( line.get() );
        }
    }

    void clear()
    {
        labels_.clear();
    }

    bool isEmpty() const
    {
        return labels_.empty();
    }

    void crop( LineNumber maxLine )
    {
        for ( auto it = labels_.begin(); it != labels_.end(); ) {
            if ( it->first >= maxLine.get() ) {
                it = labels_.erase( it );
            }
            else {
                ++it;
            }
        }
    }

    std::optional<size_t> labelFor( LineNumber line ) const
    {
        const auto label = labels_.find( line.get() );
        if ( label == labels_.end() ) {
            return {};
        }

        return label->second;
    }

private:
    std::unordered_map<uint64_t, size_t> labels_;
};

#endif
