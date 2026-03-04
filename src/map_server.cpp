#include "map_server.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>

#include "spdlog/spdlog.h"

MapServer::MapServer(QObject *parent) : QObject{parent} { initGeekplusMap(); }

void MapServer::initGeekplusMap() {
  //    qDebug()<<QCoreApplication::applicationDirPath();
  QString csvFile = QCoreApplication::applicationDirPath() +
                    QString("/resources/map/map_112.csv");

  QFile inFile(csvFile);//创建一个 QFile 对象 inFile，并传入 csvFile 作为文件路径
  QStringList lines;//创建一个空的 QStringList 对象 lines，用于存储文件的每一行数据

  if (inFile.open(QIODevice::ReadOnly | QIODevice::Text))//以只读模式打开文件
  {
    QTextStream stream_text(&inFile);//创建一个 QTextStream 对象 stream_text，并将其初始化为 inFile
    while (!stream_text.atEnd())//循环遍历 stream_text，通过 readLine() 函数逐行读取文件内容，并将每一行数据添加到 lines 中
    {
      lines.push_back(stream_text.readLine());
    }
    for (int j = 1; j < lines.size(); j++) {
      QString line = lines.at(j);
      //      spdlog::info("line {}:{}", j, line.toStdString());

      QStringList split = line.split(","); /*列数据*/
      geekplusMap.insert(split.at(0).toInt(), QPoint(split.at(1).toDouble(),
                                                     split.at(2).toDouble()));
    }
    //    qDebug() << geekplusMap;
    inFile.close();
    spdlog::info("Geekplus Map({}) loaded.", csvFile.toStdString());
  } else {
    spdlog::info("Failed to open map file:{}.", csvFile.toStdString());
  }
}

bool MapServer::getPointById(int id, QPointF &p) {
  if (geekplusMap.contains(id)) {
    spdlog::info("GetPoint By {}:{},{}", id, geekplusMap.value(id).x(),
                 geekplusMap.value(id).y());
    p = geekplusMap.value(id);
    return true;
  } else {
    spdlog::error("GetPoint By {} failed.", id);
    return false;
  }
}
