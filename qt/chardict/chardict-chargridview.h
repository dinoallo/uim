/*

Copyright (c) 2003-2011 uim Project http://code.google.com/p/uim/

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. Neither the name of authors nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

*/
#ifndef UIM_QT_CHARDICT_CHAR_GRID_VIEW_H
#define UIM_QT_CHARDICT_CHAR_GRID_VIEW_H

#include <qgridview.h>
#include <qstringlist.h>
#include <qfont.h>
#include <qpoint.h>

class CharGridView : public QGridView
{
    Q_OBJECT

public:
    CharGridView( int x, int y, QWidget *parent = 0, const char *name = 0 );
    ~CharGridView();

    void setCharacters( const QStringList &charList );
    virtual QSize sizeHint( void ) const;

    void setFont( const QFont &font ) { m_font = font; }

protected:
    virtual void paintCell( QPainter * painter, int y, int x );
    virtual void resizeEvent( QResizeEvent * e );
    virtual void contentsMousePressEvent( QMouseEvent * e );
    virtual void contentsMouseReleaseEvent( QMouseEvent * e );

    void updateCharGridView();

protected slots:
    QString coordsToChar( int x, int y );

signals:
    void charSelected( const QString & );

protected:
    QPoint m_activeCell;
    QStringList m_charList;
    QFont m_font;
};

#endif /* Not def: UIM_QT_CHARDICT_CHAR_GRID_VIEW_H */
