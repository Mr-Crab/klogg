/*
 * Copyright (C) 2021 Anton Filimonov and other contributors
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

#include "colorlabelsmanager.h"
#include "highlighterset.h"

#include <algorithm>
#include <vector>

ColorLabelsManager::ColorLabelSnapshot ColorLabelsManager::snapshot() const
{
    return { quickHighlighters_,
             std::static_pointer_cast<const LineColorLabels>( lineLabels_ ) };
}

ColorLabelsManager::ColorLabelSnapshot ColorLabelsManager::colorLabels() const
{
    return snapshot();
}

ColorLabelsManager::ColorLabelSnapshot ColorLabelsManager::clear()
{
    for ( auto& quickHighlighters : quickHighlighters_ ) {
        quickHighlighters.clear();
    }
    lineLabels_->clear();
    currentLabel_.reset();

    return snapshot();
}

ColorLabelsManager::ColorLabelSnapshot
ColorLabelsManager::setColorLabel( size_t label, const QString& text )
{
    updateColorLabel( label, text, true );
    return snapshot();
}

ColorLabelsManager::ColorLabelSnapshot
ColorLabelsManager::setNextColorLabel( const QString& text )
{
    const auto nextLabel = nextLabelIndex();
    if ( !nextLabel ) {
        return snapshot();
    }

    currentLabel_ = *nextLabel;
    updateColorLabel( *nextLabel, text, false );

    return snapshot();
}

ColorLabelsManager::ColorLabelSnapshot
ColorLabelsManager::setLineColorLabel( size_t label, const klogg::vector<LineNumber>& lines )
{
    if ( label < quickHighlighters_.size() ) {
        lineLabels_->set( label, lines );
    }
    return snapshot();
}

ColorLabelsManager::ColorLabelSnapshot
ColorLabelsManager::setNextLineColorLabel( const klogg::vector<LineNumber>& lines )
{
    const auto nextLabel = nextLabelIndex();
    if ( !nextLabel ) {
        return snapshot();
    }

    currentLabel_ = *nextLabel;
    if ( *nextLabel < quickHighlighters_.size() ) {
        lineLabels_->set( *nextLabel, lines );
    }

    return snapshot();
}

ColorLabelsManager::ColorLabelSnapshot
ColorLabelsManager::removeLineColorLabels( const klogg::vector<LineNumber>& lines )
{
    lineLabels_->remove( lines );
    return snapshot();
}

ColorLabelsManager::ColorLabelSnapshot ColorLabelsManager::clearLineLabels()
{
    lineLabels_->clear();
    return snapshot();
}

ColorLabelsManager::ColorLabelSnapshot ColorLabelsManager::cropLineLabels( LineNumber maxLine )
{
    lineLabels_->crop( maxLine );
    return snapshot();
}

std::optional<size_t> ColorLabelsManager::nextLabelIndex() const
{
    const auto& quickHighlightersConfiguration
        = HighlighterSetCollection::get().quickHighlighters();

    std::vector<size_t> cycle;
    cycle.reserve( static_cast<size_t>( quickHighlightersConfiguration.size() ) );

    for ( auto i = 0; i < quickHighlightersConfiguration.size(); ++i ) {
        if ( quickHighlightersConfiguration[ i ].useInCycle ) {
            cycle.push_back( static_cast<size_t>( i ) );
        }
    }

    if ( cycle.empty() ) {
        return {};
    }

    auto nextLabel = cycle.front();

    if ( currentLabel_.has_value() ) {
        auto nextIt = std::upper_bound( cycle.cbegin(), cycle.cend(), *currentLabel_ );
        if ( nextIt != cycle.cend() ) {
            nextLabel = *nextIt;
        }
    }

    return nextLabel;
}

ColorLabelsManager::QuickHighlightersCollection
ColorLabelsManager::updateColorLabel( size_t label, const QString& text, bool replaceCurrent )
{
    auto wasHighlightedAnyLabel = false;
    auto wasHighlightedOtherLabel = false;

    for ( auto i = 0u; i < quickHighlighters_.size(); ++i ) {
        const auto wasHighlighted = quickHighlighters_[ i ].removeAll( text ) != 0;
        wasHighlightedAnyLabel |= wasHighlighted;
        wasHighlightedOtherLabel |= ( wasHighlighted && i != label );
    }

    if ( !( wasHighlightedAnyLabel ) || ( wasHighlightedOtherLabel && replaceCurrent ) ) {
        quickHighlighters_[ label ].append( text );
    }

    return quickHighlighters_;
}
