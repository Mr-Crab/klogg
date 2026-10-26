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

#ifndef KLOGG_COLORLABELSMANAGER_H
#define KLOGG_COLORLABELSMANAGER_H

#include "abstractlogview.h"
#include "linecolorlabels.h"

#include <memory>
#include <optional>
#include <qobject.h>
#include <qobjectdefs.h>
#include <vector>

class ColorLabelsManager {
  public:
    using QuickHighlightersCollection = std::vector<AbstractLogView::QuickHighlighters>;

    struct ColorLabelSnapshot {
        QuickHighlightersCollection quickHighlighters;
        std::shared_ptr<const LineColorLabels> lineLabels;
    };

    ColorLabelSnapshot colorLabels() const;

    ColorLabelSnapshot setColorLabel( size_t label, const QString& text );
    ColorLabelSnapshot setNextColorLabel( const QString& text );

    ColorLabelSnapshot setLineColorLabel( size_t label, const klogg::vector<LineNumber>& lines );
    ColorLabelSnapshot setNextLineColorLabel( const klogg::vector<LineNumber>& lines );
    ColorLabelSnapshot removeLineColorLabels( const klogg::vector<LineNumber>& lines );

    ColorLabelSnapshot clear();
    ColorLabelSnapshot clearLineLabels();
    ColorLabelSnapshot cropLineLabels( LineNumber maxLine );

  private:
    ColorLabelSnapshot snapshot() const;
    std::optional<size_t> nextLabelIndex() const;

    QuickHighlightersCollection updateColorLabel( size_t label, const QString& text,
                                                  bool replaceCurrent );

    QuickHighlightersCollection quickHighlighters_ = QuickHighlightersCollection{9};
    std::shared_ptr<LineColorLabels> lineLabels_ = std::make_shared<LineColorLabels>();
    std::optional<size_t> currentLabel_;
};

#endif
