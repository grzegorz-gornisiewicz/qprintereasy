/**********************************************************************************
 *   QPrinterEasy                                                                 *
 *   Copyright (C) 2009                                                           *
 *   eric.maeker@free.fr, wathek@gmail.com, aurelien.french@gmail.com             *
 *                                                                                *
 * Copyright (c) 1998, Regents of the University of California                    *
 * All rights reserved.                                                           *
 * Redistribution and use in source and binary forms, with or without             *
 * modification, are permitted provided that the following conditions are met:    *
 *                                                                                *
 *     * Redistributions of source code must retain the above copyright           *
 *       notice, this list of conditions and the following disclaimer.            *
 *     * Redistributions in binary form must reproduce the above copyright        *
 *       notice, this list of conditions and the following disclaimer in the      *
 *       documentation and/or other materials provided with the distribution.     *
 *     * Neither the name of the University of California, Berkeley nor the       *
 *       names of its contributors may be used to endorse or promote products     *
 *       derived from this software without specific prior written permission.    *
 *                                                                                *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY    *
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED      *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE         *
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY   *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES     *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;   *
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND    *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT     *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS  *
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                   *
 **********************************************************************************/
#include "QPrinterEasy.h"

#include <QPainter>
#include <QRectF>
#include <QRect>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QPrintDialog>
#include <QPixmap>
#include <QSizeF>
#include <QTextBlock>
#include <QAbstractTextDocumentLayout>

// For test
#include <QTextBrowser>
#include <QDialog>
#include <QGridLayout>
#include <QDebug>
#include <QDialogButtonBox>
// End

/** \brief Only keeps some private members and private datas */
class QPrinterEasyPrivate
{
public:
    bool simpleDraw();
    bool complexDraw();

public:
    QPrinterEasyPrivate() : m_Watermark(0), m_Printer(0) {}

    QTextDocument m_Header, m_Footer, m_Content;
    QPixmap *m_Watermark;
    QPrinter *m_Printer;
    int m_HeaderPresence, m_FooterPresence;
};


QPrinterEasy::QPrinterEasy( QObject * parent )
    : QObject(parent),
    d(0)
{
    setObjectName("QPrinterEasy");
    d = new QPrinterEasyPrivate();
}

QPrinterEasy::~QPrinterEasy()
{
    if (d) {
        if ( d->m_Printer )
            delete d->m_Printer;
        d->m_Printer = 0;
        delete d;
    }
    d=0;
}

bool QPrinterEasy::askForPrinter( QWidget *parent )
{
     if ( d->m_Printer )
         delete d->m_Printer;
     d->m_Printer = new QPrinter();
     QPrintDialog *dialog = new QPrintDialog(d->m_Printer, parent);
     dialog->setWindowTitle(tr("Print Document"));
     if (dialog->exec() != QDialog::Accepted)
         return true;
     return false;
}

void QPrinterEasy::setHeader( const QString & html, const Presence p )
{
    d->m_Header.setHtml( html );
    d->m_HeaderPresence = p;
}

void QPrinterEasy::setFooter( const QString & html, const Presence p )
{
    d->m_Footer.setHtml( html );
    d->m_FooterPresence = p;
}

void QPrinterEasy::setContent( const QString & html )
{
    d->m_Content.setHtml( html );
}


bool QPrinterEasy::useDefaultPrinter()
{
    // TODO
}

bool QPrinterEasy::previewDialog( QWidget *parent, bool test )
{
    if (!d->m_Printer)
        return false;

    // For test
    if ( test ) {
        QDialog dial;
        QGridLayout g(&dial);
        QTextBrowser t(&dial);
        t.setDocument( &d->m_Content );
        g.addWidget( &t );

        QTextBrowser h(&dial);
        h.setDocument( &d->m_Header );
        g.addWidget( &h );

        QTextBrowser f(&dial);
        f.setDocument( &d->m_Footer );
        g.addWidget( &f );

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                           | QDialogButtonBox::Cancel);

        connect(buttonBox, SIGNAL(accepted()), &dial, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), &dial, SLOT(reject()));
        g.addWidget( buttonBox );
        dial.exec();
        // end of test
    }

    QPrintPreviewDialog dialog(d->m_Printer, parent);
    connect( &dialog, SIGNAL(paintRequested(QPrinter *)), this, SLOT(print(QPrinter *)) );
    dialog.exec();
    return true;
}

bool QPrinterEasy::print( QPrinter *printer )
{
    if ( (!printer) && (!d->m_Printer) )
        return false;
    if (!printer)
        printer = d->m_Printer;

    // test if we can use the simpleDraw member to add header/footer to each pages
    if ( ( d->m_HeaderPresence == OnEachPages) &&
         ( d->m_FooterPresence == OnEachPages) &&
         ( d->m_Watermark == 0 ) )
        return d->simpleDraw();
    else
        return d->complexDraw();
}

bool QPrinterEasyPrivate::complexDraw()
{
    qWarning() << "complexDraw";

    // Here we have to print different documents :
    // 'content' is the main document to print
    // 'header' the header, see m_HeaderPresence to know when to add the header
    // 'footer' the footer, see m_FooterPresence to know when to add the footer

    // This function draw the content block by block
    int pageWidth = m_Printer->pageRect().width() - 20;                     //TODO add margins
    m_Header.setTextWidth( pageWidth );
    m_Footer.setTextWidth( pageWidth );
    m_Content.setTextWidth( pageWidth );

    int headerHeight, footerHeight;

//    TODO use iterators
//    QTextBlock::Iterator it = m_Content.begin().begin();
//    QTextBlock::Iterator lastBlock = m_Content.lastBlock().end();

    int i = 0;
    int page = 1;
    QTextBlock block = m_Content.begin();
    while (block.isValid()) {
        // new page ?
        // do we have to include the header ?
        headerHeight = m_Header.size().height();
        // do we have to include the footer ?
        footerHeight = m_Footer.size().height();

        // warn boundingrect
        qWarning() << m_Content.documentLayout()->blockBoundingRect(block);//block.layout()->boundingRect();

        // next block
        block = block.next();
        ++i;
    }

    qWarning() << m_Content.blockCount() << i;

    // Need to be recalculated for each pages
    // headerHeight, footerHeight, innerRect, currentRect, contentRect
    QSize size;
    size.setHeight( m_Printer->pageRect().height() - headerHeight - footerHeight );
    size.setWidth( pageWidth );
    m_Content.setPageSize(size);
    m_Content.setUseDesignMetrics(true);

    // prepare drawing areas
    QRect innerRect = m_Printer->pageRect();                                     // the content area
    innerRect.setTop(innerRect.top() + headerHeight );
    innerRect.setBottom(innerRect.bottom() - footerHeight );
    QRect currentRect = QRect(QPoint(0,0), innerRect.size());                  // content area
    QRect contentRect = QRect(QPoint(0,0), m_Content.size().toSize() );     // whole document drawing rectangle

    QPainter painter(m_Printer);
    int pageNumber = 0;

    // moving into Painter : go under the header
    painter.save();
    painter.translate(0, headerHeight);
    while (currentRect.intersects(contentRect)) {
        // draw content for this page
        m_Content.drawContents(&painter, currentRect);
        pageNumber++;
        currentRect.translate(0, currentRect.height());
        // moving into Painter : return to the beginning of the page
        painter.restore();

        // draw header
        QRectF headRect = QRectF(QPoint(0,0), m_Header.size() );
//        painter.drawRect( headRect );
        m_Header.drawContents(&painter, headRect );

        // draw footer
        painter.save();
        painter.translate(0, m_Printer->pageRect().bottom() - footerHeight - 10);
        QRectF footRect = QRectF(QPoint(0,0), m_Footer.size() );
//        painter.drawRect( footRect );
        m_Footer.drawContents(&painter, footRect);
        painter.restore();

        // calculate new page
        painter.save();
        painter.translate(0, -currentRect.height() * pageNumber + headerHeight);

        if (currentRect.intersects(contentRect))
            m_Printer->newPage();
    }
    painter.restore();
    painter.end();

    return true;
}


bool QPrinterEasyPrivate::simpleDraw()
{
    // This private member is used when there is only one header and one footer to add on each pages
    // All pages measures
    // pageWidth, margins of document, margins of printer
    int pageWidth = m_Printer->pageRect().width() - 20;                     //TODO add margins

    // Need to be recalculated for each pages
    // headerHeight, footerHeight, innerRect, currentRect, contentRect
    m_Header.setTextWidth( pageWidth );
    m_Footer.setTextWidth( pageWidth );
    int headerHeight = m_Header.size().height();
    int footerHeight = m_Footer.size().height();
    QSize size;
    size.setHeight( m_Printer->pageRect().height() - headerHeight - footerHeight );
    size.setWidth( pageWidth );
    m_Content.setPageSize(size);
    m_Content.setUseDesignMetrics(true);

    // prepare drawing areas
    QRect innerRect = m_Printer->pageRect();                                     // the content area
    innerRect.setTop(innerRect.top() + headerHeight );
    innerRect.setBottom(innerRect.bottom() - footerHeight );
    QRect currentRect = QRect(QPoint(0,0), innerRect.size());                  // content area
    QRect contentRect = QRect(QPoint(0,0), m_Content.size().toSize() );     // whole document drawing rectangle

    QPainter painter(m_Printer);
    int pageNumber = 0;

    // moving into Painter : go under the header
    painter.save();
    painter.translate(0, headerHeight);
    while (currentRect.intersects(contentRect)) {
        // draw content for this page
        m_Content.drawContents(&painter, currentRect);
        pageNumber++;
        currentRect.translate(0, currentRect.height());
        // moving into Painter : return to the beginning of the page
        painter.restore();

        // draw header
        QRectF headRect = QRectF(QPoint(0,0), m_Header.size() );
//        painter.drawRect( headRect );
        m_Header.drawContents(&painter, headRect );

        // draw footer
        painter.save();
        painter.translate(0,m_Printer->pageRect().bottom() - footerHeight - 10);
        QRectF footRect = QRectF(QPoint(0,0), m_Footer.size() );
//        painter.drawRect( footRect );
        m_Footer.drawContents(&painter, footRect);
        painter.restore();

        // calculate new page
        painter.save();
        painter.translate(0, -currentRect.height() * pageNumber + headerHeight);

        if (currentRect.intersects(contentRect))
            m_Printer->newPage();
    }
    painter.restore();
    painter.end();
    return true;
}
