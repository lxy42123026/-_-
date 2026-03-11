#ifndef MAPSERVER_H
#define MAPSERVER_H

#include <QHash>
#include <QObject>
#include <QPointF>

class MapServer : public QObject {
  Q_OBJECT
 public:
  explicit MapServer(QObject *parent = nullptr);

  bool getPointById(int id, QPointF &p);

 signals:

 private:
  void initGeekplusMap();
  QHash<int, QPointF> geekplusMap;
};

#endif  // MAPSERVER_H
