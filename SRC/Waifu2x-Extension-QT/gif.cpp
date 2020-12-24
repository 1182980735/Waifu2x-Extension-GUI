﻿/*
    Copyright (C) 2020  Aaron Feng

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    My Github homepage: https://github.com/AaronFeng753
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"

/*
获取gif帧间隔时间
*/
int MainWindow::Gif_getDuration(QString gifPath)
{
    QString program = Current_Path+"/python_ext_waifu2xEX.exe";
    QProcess GifDuration;
    GifDuration.start("\""+program+"\" \""+gifPath+"\" duration");
    while(!GifDuration.waitForStarted(100)&&!QProcess_stop) {}
    while(!GifDuration.waitForFinished(100)&&!QProcess_stop) {}
    int Duration=GifDuration.readAllStandardOutput().toInt();
    if(Duration<=0)
    {
        QMovie movie(gifPath);
        movie.setSpeed(1);
        movie.start();
        movie.stop();
        Duration = ((movie.nextFrameDelay()/100)+1)/10;
    }
    return Duration;
}
/*
获取gif帧数量的位数
*/
int MainWindow::Gif_getFrameDigits(QString gifPath)
{
    QMovie movie(gifPath);
    int FrameCount=1+(int)log10(movie.frameCount());//获取frame位数
    return FrameCount;
}
/*
拆分gif
*/
void MainWindow::Gif_splitGif(QString gifPath,QString SplitFramesFolderPath)
{
    emit Send_TextBrowser_NewMessage(tr("Start splitting GIF:[")+gifPath+"]");
    int FrameDigits = Gif_getFrameDigits(gifPath);
    //删除并新建帧文件夹
    file_DelDir(SplitFramesFolderPath);
    file_mkDir(SplitFramesFolderPath);
    //开始用convert处理
    QString program = Current_Path+"/convert_waifu2xEX.exe";
    QString cmd = "\"" + program + "\"" + " -coalesce " + "\"" + gifPath + "\"" + " " + "\"" + SplitFramesFolderPath + "/%0"+QString::number(FrameDigits,10)+"d.png\"";
    QProcess *SplitGIF=new QProcess();
    SplitGIF->start(cmd);
    while(!SplitGIF->waitForStarted(100)&&!QProcess_stop) {}
    while(!SplitGIF->waitForFinished(100)&&!QProcess_stop) {}
    if(file_isDirEmpty(SplitFramesFolderPath))//如果拆分失败,尝试win7兼容指令
    {
        QString cmd = "\"" + program + "\"" + " -coalesce " + "\"" + gifPath + "\"" + " " + "\"" + SplitFramesFolderPath + "/%%0"+QString::number(FrameDigits,10)+"d.png\"";
        QProcess *SplitGIF=new QProcess();
        SplitGIF->start(cmd);
        while(!SplitGIF->waitForStarted(100)&&!QProcess_stop) {}
        while(!SplitGIF->waitForFinished(100)&&!QProcess_stop) {}
    }
    emit Send_TextBrowser_NewMessage(tr("Finish splitting GIF:[")+gifPath+"]");
}
/*
组装gif
*/
void MainWindow::Gif_assembleGif(QString ResGifPath,QString ScaledFramesPath,int Duration,bool CustRes_isEnabled,int CustRes_height,int CustRes_width)
{
    emit Send_TextBrowser_NewMessage(tr("Start to assemble GIF:[")+ResGifPath+"]");
    //===============================
    QString resize_cmd ="";
    if(CustRes_isEnabled)
    {
        if(CustRes_AspectRatioMode==Qt::IgnoreAspectRatio)
        {
            resize_cmd =" -resize "+QString::number(CustRes_width,10)+"x"+QString::number(CustRes_height,10)+"! ";
        }
        if(CustRes_AspectRatioMode==Qt::KeepAspectRatio)
        {
            resize_cmd =" -resize "+QString::number(CustRes_width,10)+"x"+QString::number(CustRes_height,10)+" ";
        }
        if(CustRes_AspectRatioMode==Qt::KeepAspectRatioByExpanding)
        {
            if(CustRes_width>CustRes_height)
            {
                resize_cmd =" -resize "+QString::number(CustRes_width,10)+" ";
            }
            else
            {
                resize_cmd =" -resize x"+QString::number(CustRes_height,10)+" ";
            }
        }
    }
    QString program = Current_Path+"/convert_waifu2xEX.exe";
    QString cmd = "\"" + program + "\" "+resize_cmd+" -delay " + QString::number(Duration, 10) + " -loop 0 " + "\"" + ScaledFramesPath + "/*png\" \""+ResGifPath+"\"";
    QProcess *AssembleGIF=new QProcess();
    AssembleGIF->start(cmd);
    while(!AssembleGIF->waitForStarted(100)&&!QProcess_stop) {}
    while(!AssembleGIF->waitForFinished(100)&&!QProcess_stop) {}
    //======= 纠正文件名称错误(当 结果gif文件路径内有 % 符号时) ======
    if(QFile::exists(ResGifPath)==false)
    {
        QFileInfo fileinfo(ResGifPath);
        QString file_name = file_getBaseName(ResGifPath);
        QString file_ext = fileinfo.suffix();
        QString file_path = file_getFolderPath(fileinfo);
        QString ActualResGifPath = file_path + "/" + file_name + "-0." + file_ext;
        if(QFile::exists(ActualResGifPath)==true)
        {
            QFile::rename(ActualResGifPath,ResGifPath);
        }
    }
    //===========
    emit Send_TextBrowser_NewMessage(tr("Finish assembling GIF:[")+ResGifPath+"]");
}
/*
压缩gif
*/
QString MainWindow::Gif_compressGif(QString gifPath,QString gifPath_compressd)
{
    emit Send_TextBrowser_NewMessage(tr("Starting to optimize GIF:[")+gifPath+"]");
    //=====
    QString program = Current_Path+"/gifsicle_waifu2xEX.exe";
    QString cmd = "\"" + program + "\"" + " -O3 -i \""+gifPath+"\" -o \""+gifPath_compressd+"\"";
    QProcess *CompressGIF=new QProcess();
    CompressGIF->start(cmd);
    while(!CompressGIF->waitForStarted(100)&&!QProcess_stop) {}
    while(!CompressGIF->waitForFinished(100)&&!QProcess_stop) {}
    //======
    //判断是否生成压缩后的gif
    if(QFile::exists(gifPath_compressd) == false)
    {
        emit Send_TextBrowser_NewMessage(tr("Error occured when processing [")+gifPath+tr("]. Error: [Can't optimize gif.]"));
        return gifPath;//返回源文件路径
    }
    //======
    //比较文件大小,判断压缩是否有效
    QFileInfo *gifPath_QFileInfo = new QFileInfo(gifPath);
    QFileInfo *gifPath_compressd_QFileInfo = new QFileInfo(gifPath_compressd);
    if((gifPath_compressd_QFileInfo->size() < gifPath_QFileInfo->size())==false)
    {
        emit Send_TextBrowser_NewMessage(tr("Failed to optimize gif [")+gifPath+tr("] to reduce storage usage, the optimized gif file will be deleted."));
        QFile::remove(gifPath_compressd);
        return gifPath;//返回源文件路径
    }
    //======
    QFile::remove(gifPath);
    emit Send_TextBrowser_NewMessage(tr("Finish optimizing GIF:[")+gifPath+"]");
    return gifPath_compressd;//返回处理完成的文件路径
}
