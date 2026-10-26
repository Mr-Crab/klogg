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

#include <catch2/catch.hpp>

#include "colorlabelsmanager.h"
#include "linetypes.h"

SCENARIO( "Color labels manager stores line labels", "[colorlabels]" )
{
    GIVEN( "An empty color labels manager" )
    {
        ColorLabelsManager manager;

        WHEN( "Setting a label on a range of source lines" )
        {
            const auto snapshot = manager.setLineColorLabel(
                2, klogg::vector<LineNumber>{ 1_lnum, 3_lnum, 5_lnum } );

            THEN( "All lines have the requested label" )
            {
                REQUIRE( snapshot.lineLabels );
                REQUIRE( snapshot.lineLabels->labelFor( 1_lnum ) == 2 );
                REQUIRE( snapshot.lineLabels->labelFor( 3_lnum ) == 2 );
                REQUIRE( snapshot.lineLabels->labelFor( 5_lnum ) == 2 );
                REQUIRE_FALSE( snapshot.lineLabels->labelFor( 2_lnum ).has_value() );
            }
        }

        WHEN( "Setting a different label on an already labeled line" )
        {
            manager.setLineColorLabel( 1, klogg::vector<LineNumber>{ 7_lnum } );
            const auto snapshot
                = manager.setLineColorLabel( 4, klogg::vector<LineNumber>{ 7_lnum } );

            THEN( "The new label replaces the old one" )
            {
                REQUIRE( snapshot.lineLabels->labelFor( 7_lnum ) == 4 );
            }
        }

        WHEN( "Removing labels from selected lines" )
        {
            manager.setLineColorLabel( 3, klogg::vector<LineNumber>{ 2_lnum, 4_lnum } );
            const auto snapshot
                = manager.removeLineColorLabels( klogg::vector<LineNumber>{ 2_lnum } );

            THEN( "Only the requested line is unlabeled" )
            {
                REQUIRE_FALSE( snapshot.lineLabels->labelFor( 2_lnum ).has_value() );
                REQUIRE( snapshot.lineLabels->labelFor( 4_lnum ) == 3 );
            }
        }

        WHEN( "Cropping labels beyond the end of the log" )
        {
            manager.setLineColorLabel( 5, klogg::vector<LineNumber>{ 1_lnum, 9_lnum, 12_lnum } );
            const auto snapshot = manager.cropLineLabels( 10_lnum );

            THEN( "Labels at or after the limit are removed" )
            {
                REQUIRE( snapshot.lineLabels->labelFor( 1_lnum ) == 5 );
                REQUIRE( snapshot.lineLabels->labelFor( 9_lnum ) == 5 );
                REQUIRE_FALSE( snapshot.lineLabels->labelFor( 12_lnum ).has_value() );
            }
        }

        WHEN( "Clearing only line labels" )
        {
            manager.setLineColorLabel( 6, klogg::vector<LineNumber>{ 8_lnum } );
            const auto snapshot = manager.clearLineLabels();

            THEN( "The line label is gone" )
            {
                REQUIRE_FALSE( snapshot.lineLabels->labelFor( 8_lnum ).has_value() );
            }
        }
    }
}
