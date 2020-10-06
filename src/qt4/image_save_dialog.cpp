// -*-c++-*-

/*!
  \file image_save_dialog.cpp
  \brief Image save dialog class Source File.
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QtGui>

#include "image_save_dialog.h"

#include "main_data.h"
#include "main_window.h"
#include "field_canvas.h"
#include "view_holder.h"
#include "dir_selector.h"

#include "options.h"

#include <arpa/inet.h>

// Lea Eisti 2018
// If we want the image to be analysed by the python model : true
const int SITUATION_SCORE = true;

/*-------------------------------------------------------------------*/
/*!

*/
ImageSaveDialog::ImageSaveDialog( MainWindow * main_window,
                                  FieldCanvas * field_canvas,
                                  MainData & main_data )
    : QDialog( main_window )
    , M_main_window( main_window )
    , M_field_canvas( field_canvas )
    , M_main_data( main_data )

{
    assert( main_window );

    this->setWindowTitle( tr( "Image Save" ) );

    createControls();

}

/*-------------------------------------------------------------------*/
/*!

*/
// Lea Eisti 2018
//To do the screenshots automatically
ImageSaveDialog::ImageSaveDialog( MainWindow * main_window,
                                  FieldCanvas * field_canvas,
                                  MainData & main_data,
                                  int current_index,
                                  QString M_dateTime_begin )
    : QDialog( main_window )
    , M_main_window( main_window )
    , M_field_canvas( field_canvas )
    , M_main_data( main_data )
    , M_current_index ( current_index )

{

    const MonitorViewConstPtr latest = main_data.viewHolder().latestViewData();

    QString saveDir = QDir::currentPath()+"/data/images/";

    QString namePrefix = "frame-";
    QString formatName = "png";

    saveDir += M_dateTime_begin;

    QString left_team = QString::fromStdString( latest->leftTeam().name() );
    QString left_score = QString::number( latest->leftTeam().score() );

    QString right_team = QString::fromStdString( latest->rightTeam().name() );
    QString right_score = QString::number( latest->rightTeam().score() );

    saveDir += left_team;
    saveDir += tr( "-vs-" );
    saveDir += right_team;

    saveImage( current_index,
               saveDir,
               namePrefix,
               formatName );

}


/*-------------------------------------------------------------------*/
/*!

*/
ImageSaveDialog::~ImageSaveDialog()
{
    //std::cerr << "delete ImageSaveDialog" << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ImageSaveDialog::createControls()
{
    QVBoxLayout * layout = new QVBoxLayout();
    layout->setSizeConstraint( QLayout::SetFixedSize );

    //display the cycle range we can select
    layout->addWidget( createCycleSelectControls(),
                       0, Qt::AlignLeft );

    //display the information about the file we can write/select
    layout->addWidget( createFileNameControls(),
                       0, Qt::AlignLeft );


    layout->addWidget( createDirSelectControls(),
                       0, Qt::AlignLeft );

    layout->addLayout( createExecuteControls(),
                       0 );

    this->setLayout( layout );
}

/*-------------------------------------------------------------------*/
/*!

*/
QWidget *
ImageSaveDialog::createCycleSelectControls()
{
    QGroupBox * group_box = new QGroupBox( tr( "Cycle Range" ) );

    QHBoxLayout * layout = new QHBoxLayout();
    layout->setSpacing( 0 );

    // create input spinctrls & labels
    const std::vector< MonitorViewPtr > & view
        = M_main_data.viewHolder().monitorViewCont();
    int min_cycle = 0;
    int max_cycle = 0;
    if ( ! view.empty() )
    {
        min_cycle = static_cast< int >( view.front()->cycle() );
        max_cycle = static_cast< int >( view.back()->cycle() );
    }

    layout->addWidget( new QLabel( tr( "Start: " ) ),
                       0, Qt::AlignVCenter );

    M_start_cycle = new QSpinBox();
    M_start_cycle->setRange( min_cycle, max_cycle );
    M_start_cycle->setValue( min_cycle );
    layout->addWidget( M_start_cycle,
                       0, Qt::AlignVCenter );

    layout->addWidget( new QLabel( tr( " - End: " ) ),
                       0, Qt::AlignVCenter );

    M_end_cycle = new QSpinBox();
    M_end_cycle->setRange( min_cycle, max_cycle );
    M_end_cycle->setValue( min_cycle );
    layout->addWidget( M_end_cycle,
                       0, Qt::AlignVCenter );

    layout->addSpacing( 4 );

    QPushButton * select_all_btn = new QPushButton( tr( "Select All" ) );
    select_all_btn->setMaximumSize( 80, this->fontMetrics().height() + 12 );
    connect( select_all_btn, SIGNAL( clicked() ),
             this, SLOT( selectAllCycle() ) );
    layout->addWidget( select_all_btn,
                       0, Qt::AlignVCenter );

    group_box->setLayout( layout );
    return group_box;
}

/*-------------------------------------------------------------------*/
/*!

*/
QWidget *
ImageSaveDialog::createFileNameControls()
{
    QGroupBox * group_box = new QGroupBox( tr( "File" ) );

    QHBoxLayout * layout = new QHBoxLayout();
    layout->setSpacing( 0 );

    layout->addWidget( new QLabel( tr( "Name Prefix: " ) ),
                       0, Qt::AlignVCenter );

    M_name_prefix = new QLineEdit( tr( "image-" ) );
    if ( ! Options::instance().imageNamePrefix().empty() )
    {
        M_name_prefix->setText( Options::instance().imageNamePrefix().c_str() );
    }
    M_name_prefix->setMaximumSize( 80, 24 );
    layout->addWidget( M_name_prefix,
                       0, Qt::AlignVCenter );

    layout->addWidget( new QLabel( tr( " Format: " ) ),
                       0, Qt::AlignVCenter );

    const QString default_format
        = ( Options::instance().imageSaveFormat().empty()
            ? QString::fromAscii( "PNG" )
            : QString::fromAscii( Options::instance().imageSaveFormat().c_str() ) );

    M_format_choice = new QComboBox();
    {
        int i = 0;
        int png_index = 0;
        int max_width = 0;
        Q_FOREACH( QByteArray format, QImageWriter::supportedImageFormats() )
            {
                QString text = tr( "%1" ).arg( QString( format ).toUpper() );
                int width = this->fontMetrics().width( text );
                if ( max_width < width )
                {
                    max_width = width;
                }

                if ( text == default_format )
                {
                    png_index = i;
                }
                ++i;
                M_format_choice->addItem( text );
            }
        M_format_choice->setCurrentIndex( png_index );
        M_format_choice->setMaximumWidth( max_width + 40 );
    }
    layout->addWidget( M_format_choice,
                       0, Qt::AlignVCenter );

    group_box->setLayout( layout );
    return group_box;;
}

/*-------------------------------------------------------------------*/
/*!

*/
QWidget *
ImageSaveDialog::createDirSelectControls()
{
    QGroupBox * group_box = new QGroupBox( tr( "Save Directry" ) );

    QHBoxLayout * layout = new QHBoxLayout();

    QString dir_str;
    if ( ! Options::instance().imageSaveDir().empty() )
    {
        QFileInfo file_info( Options::instance().imageSaveDir().c_str() );
        if ( ! file_info.isDir() )
        {
            QDir dir;
            if ( dir.mkdir( file_info.absoluteFilePath() ) )
            {
                file_info.setFile( file_info.absoluteFilePath() );
            }
        }

        if ( file_info.isDir()
             && file_info.isWritable() )
        {
            dir_str = file_info.absoluteFilePath();
        }
    }
    else
    {
        dir_str = QDir::currentPath();
    }

    M_saved_dir = new QLineEdit( dir_str );
    M_saved_dir->setMinimumWidth( qMax( 360,
                                        M_saved_dir->fontMetrics().width( dir_str )
                                        + 32 ) );
    layout->addWidget( M_saved_dir,
                       0, Qt::AlignVCenter );

    QPushButton * button = new QPushButton( tr( "..." ) );
    button->setMaximumSize( this->fontMetrics().width( tr( "..." ) ) + 12,
                            this->fontMetrics().height() + 12 );
    button->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    connect( button, SIGNAL( clicked() ),
             this, SLOT( selectSavedDir() ) );
    layout->addWidget( button );

    group_box->setLayout( layout );

    return group_box;
}

/*-------------------------------------------------------------------*/
/*!

*/
QLayout *
ImageSaveDialog::createExecuteControls()
{
    QHBoxLayout * layout = new QHBoxLayout();

    QPushButton * button = new QPushButton( tr( "Create!" ) );
    connect( button, SIGNAL( clicked() ),
             this, SLOT( executeSave() ) );
    layout->addWidget( button,
                       10, Qt::AlignVCenter );

    QPushButton * cancel = new QPushButton( tr( "Cancel" ) );
    connect( cancel, SIGNAL( clicked() ),
             this, SLOT( reject() ) );
    layout->addWidget( cancel,
                       2, Qt::AlignVCenter );

    return layout;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ImageSaveDialog::showEvent( QShowEvent * event )
{
    const std::vector< MonitorViewPtr > & view
        = M_main_data.viewHolder().monitorViewCont();

    int min_cycle = 0;
    int max_cycle = 0;
    if ( ! view.empty() )
    {
        min_cycle = static_cast< int >( view.front()->cycle() );
        max_cycle = static_cast< int >( view.back()->cycle() );
    }
    M_start_cycle->setRange( min_cycle, max_cycle );
    M_end_cycle->setRange( min_cycle, max_cycle );

    event->accept();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ImageSaveDialog::selectAllCycle()
{
    const std::vector< MonitorViewPtr > & view
        = M_main_data.viewHolder().monitorViewCont();
    int min_cycle = 0;
    int max_cycle = 0;
    if ( ! view.empty() )
    {
        min_cycle = static_cast< int >( view.front()->cycle() );
        max_cycle = static_cast< int >( view.back()->cycle() );
    }

    M_start_cycle->setValue( min_cycle );
    M_end_cycle->setValue( max_cycle );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ImageSaveDialog::selectSavedDir()
{
    /*
    DirSelector dlg( this,
                     tr( "Choose a save directory" ),
                     M_saved_dir->text() ); // default path

    if ( ! dlg.exec() )
    {
        return;
    }

    M_saved_dir->setText( dlg.dirPath() );
    */

    QString dir
        = QFileDialog::getExistingDirectory( this,
                                             tr( "Choose a saved directory" ),
                                             M_saved_dir->text(),
                                             QFileDialog::ShowDirsOnly
                                             | QFileDialog::DontResolveSymlinks);

    M_saved_dir->setText( dir );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ImageSaveDialog::executeSave()
{
    if ( M_saved_dir->text().isEmpty() )
    {
        QMessageBox::warning( this,
                              tr( "Error" ),
                              tr( "Empty directry name!" ) );
        return;
    }

    /*
    std::cout << "Create button pressed" << std::endl;
    std::cout << "  start cycle = " << M_start_cycle->GetValue()
              << "  end cycle = " << M_end_cycle->GetValue()
              << std::endl;
    std::cout << "  name prefix = " << M_name_prefix->GetValue()
              << std::endl;
    std::cout << "  format = " << format_choices[M_format_choice->GetSelection()]
              << std::endl;
    std::cout << "  save dir = " << M_saved_dir->GetValue()
              << std::endl;
    */
    QString format = M_format_choice->currentText();

    saveImage( M_start_cycle->value(),
               M_end_cycle->value(),
               M_saved_dir->text(),
               M_name_prefix->text(),
               M_format_choice->currentText() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ImageSaveDialog::saveImage( const int start_cycle,
                            const int end_cycle,
                            const QString & saved_dir,
                            const QString & name_prefix,
                            const QString & format_name )
{
    // if there is no data for the current match (for example when we don't start the server)
    // = if the data container is empty
    if ( M_main_data.viewHolder().monitorViewCont().empty() )
    {
        QMessageBox::warning( this,
                              tr( "Error" ),
                              tr( "No Monitor View Data!" ) );
        reject();
        return;
    }

    QString format = format_name.toLower();

    // return the size of the object M_main_data
    const int backup_index = M_main_data.viewIndex();

    const int first = M_main_data.viewHolder().getIndexOf( start_cycle );
    const int last = M_main_data.viewHolder().getIndexOf( end_cycle );

    if ( first > last )
    {
        QMessageBox::warning( this,
                              tr( "Error" ),
                              tr( "Invalid cycle range!" ) );
        return;
    }

    // create file path base
    QString file_path = saved_dir;

    if ( ! file_path.endsWith( QChar( '/' ) ) )
    {
        // tr : available with different languages
        file_path += tr( "/" );
    }

    // check directory
    {
        QDir dir( file_path );
        if ( ! dir.exists()
             && ! dir.mkpath( file_path ) )
        {
            QMessageBox::warning( this,
                                  tr( "Error" ),
                                  tr( "Failed to create image save directory!" ) );
            return;
        }
    }

    {
        // permit to have the prefix written correctly
        QString name_prefix_trim = name_prefix;
        while ( ! name_prefix_trim.isEmpty()
                && name_prefix_trim.endsWith( QChar( '/' ) ) )
        {
            name_prefix_trim.remove( name_prefix_trim.length() - 1, 1 );
        }

        file_path += name_prefix_trim;
    }

    QString file_ext = tr( "." ) + format;

    // field_canvas : data of the field
    const QSize size = M_field_canvas->size();
    QImage image( M_field_canvas->size(), QImage::Format_RGB32 );

    std::cerr << "Save image size = "
              << M_field_canvas->size().width() << ", "
              << M_field_canvas->size().height() << std::endl;

    // constructs a painter that begins painting the paint "image" immediately.
    QPainter painter( &image );

    // show progress dialog
    QProgressDialog progress_dialog( this );
    progress_dialog.setWindowTitle( tr( "Image Save Progress" ) );
    progress_dialog.setRange( first, last );
    progress_dialog.setValue( first );
    progress_dialog.setLabelText( file_path + tr( "00000" ) + file_ext );

    bool confirm = true;

    // main loop
    for ( int i = first; i <= last; ++i )
    {
        // full file path
        QString file_path_all = file_path;
        //snprintf( count, 16, "%05d", i );
        file_path_all += QString( "%1" ).arg( i, 5, 10, QChar( '0' ) );
        file_path_all += file_ext;

        if ( confirm
             && QFile::exists( file_path_all ) )
        {
            int result
                = QMessageBox::question( this,
                                         tr( "Overwrite?" ),
                                         tr( "There already exists a file called %1.\n Overwrite?")
                                         .arg( file_path_all ),
                                         QMessageBox::No,
                                         QMessageBox::Yes,
                                         QMessageBox::YesAll );
            if ( result == QMessageBox::No )
            {
                progress_dialog.cancel();
                M_main_data.setViewDataIndex( backup_index );
                return;
            }
            else if ( result == QMessageBox::YesAll )
            {
                confirm = false;
            }
        }

        progress_dialog.setValue( i );
        progress_dialog.setLabelText( file_path_all );

        //QCoreApplication::processEvents();
        if ( i % 20 == 0 )
        {
            qApp->processEvents();
            if ( progress_dialog.wasCanceled() )
            {
                M_main_data.setViewDataIndex( backup_index );
                return;
            }
        }

        M_main_data.setViewDataIndex( i );
        M_main_data.update( size.width(), size.height() );

        M_field_canvas->draw( painter );

        // std::cout << "save image " << file_path << std::endl;
        if ( ! image.save( file_path_all, format.toAscii() ) )
        {
            QMessageBox::critical( this,
                                   tr( "Error" ),
                                   tr( "Failed to save image file " )
                                   + file_path_all );
            M_main_data.setViewDataIndex( backup_index );
            return;
        }
    }

    M_main_data.setViewDataIndex( backup_index );

    accept();
}

/*-------------------------------------------------------------------*/
/*!

*/
/*
* Modification and comments : Lea Eisti 2018
* current_index : index of the frame we want
* saved_dir : the directory where the images are saved
* name_prefix : the name of the image ("image-")
* format_name : the format of the image ("png")
*
* viewHolder : all the things we can see in the window (play mode, team, ...)
* monitorViewCont : return the M_monitor_view_cont, with the type : std::vector<MonitorViewPtr>.
*   M_monitor_view_cont is the data container.
* main_data : M_main_data : parameter passed to the constructor of ImageSaveDialog.
*   Containt the data of the matches. Many manipulation with the class log_player.
*/
void
ImageSaveDialog::saveImage( const int current_index,
                            const QString & saved_dir,
                            const QString & name_prefix,
                            const QString & format_name )
{
    // if there is no data for the current match (for example when we don't start the server)
    // = if the data container is empty
    if ( M_main_data.viewHolder().monitorViewCont().empty() )
    {
        QMessageBox::warning( this,
                              tr( "Error" ),
                              tr( "No Monitor View Data!" ) );
        reject();
        return;
    }

    QString format = format_name.toLower();

    // create file path base
    QString file_path = saved_dir;

    if ( ! file_path.endsWith( QChar( '/' ) ) )
    {
        // tr : available with different languages
        file_path += tr( "/" );
    }

    // check directory
    {
        QDir dir( file_path );
        if ( ! dir.exists()
             && ! dir.mkpath( file_path ) )
        {
            QMessageBox::warning( this,
                                  tr( "Error" ),
                                  tr( "Failed to create image save directory!" ) );
            return;
        }
    }

    {
        // permit to have the prefix written correctly
        QString name_prefix_trim = name_prefix;
        while ( ! name_prefix_trim.isEmpty()
                && name_prefix_trim.endsWith( QChar( '/' ) ) )
        {
            name_prefix_trim.remove( name_prefix_trim.length() - 1, 1 );
        }

        file_path += name_prefix_trim;
    }

    QString file_ext = tr( "." ) + format;

    // 2020/10 Fukushima changed
    // To have the same image that the Python model wants
    QImage image( QSize( 256, 160 ), QImage::Format_RGB32 );

    // constructs a painter that begins painting the paint "image" immediately.
    QPainter painter( &image );

    // full file path
    QString file_path_all = file_path;

    file_path_all += QString( "%1" ).arg( current_index, 5, 10, QChar( '0' ) );
    file_path_all += file_ext;

    // 2020/10 Fukushima changed
    // To have the same image that the Python model wants
    M_main_data.update( 256, 160 );
    M_field_canvas->draw( painter );

    // Fukushima 2020 comment out (this implementation cause a bug)
    // Lea Eisti 2018
    // To have the same image size that the Python model wants
    // painter.scale(0.2,0.2);
    // QImage image3 = image.scaled(256,160,Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    if ( ! image.save( file_path_all, format.toAscii() ) )
    {
        QMessageBox::critical( this,
                               tr( "Error" ),
                               tr( "Failed to save image file " )
                               + file_path_all );
        return;
    }

    accept();


    // Lea Eisti 2018
    if (SITUATION_SCORE)
    {

        // adress port
        int PORT =  15555;
        int sock;
        /*
        struct sockaddr_in : data type for addresses
        struct sockaddr_in {
            unsigned short sin_family; : Internet protocol (AF_INET)
            unsigned short sin_port;  : Address port (16 bits)
            struct in_addr sin_addr; : Internet address (32 bits)
            char sin_zero[8];  : Not used
        }
        */
        struct sockaddr_in serv_addr;

        // We create the socket :
        /*
        int sock = socket(family, type, protocol);
            sock: socket descriptor, an integer
            family : integer, communication domain (AF_INET : IPv4 addresses)
            type : communication type (SOCK_STREAM : connection-based service, TCP socket)
            protocol : specifies protocol (0 : default protocol, here TCP because of the type)
        return -1 if fail.
        */
        if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("Creation failed : ");
        }

        // We configure the connexion
        // inet_addr("127.0.0.1") : we set the socket address to localhost
        serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);

        // The client establishes a connection with the server by calling connect()
        /*
        connect(sock, &foreignAddr, addrlen);
            sock : integer, socket to be used in connection
            foreignAddr: struct sockaddr: address of the passive participant
            addrlen: integer, sizeof(foreignAddr)
        return -1 if fail.
        */
        if( ::connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0 )
        {
            perror("Connection failed : ");
        }

        // We want to send the name of the directory where the images are saved.
        std::string buffer1 = saved_dir.toStdString();
        char buffer[256];
        strcpy(buffer,buffer1.c_str());


        // We send the data
        /*
        send(sock, msg, msgLen, flags);
            msg: const void[], message to be transmitted
            msgLen: integer, length of message (in bytes) to transmit
            flags: integer, special options, usually just 0
        return the number of bytes transmitted. -1 if error
        */
        if (send(sock, buffer, strlen(buffer), 0) < 0)
        {
            perror("Send failed : ");
        }

        // We close the socket
        ::close(sock);

    }

}
