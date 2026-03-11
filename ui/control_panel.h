#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QTimer>

#include "AgvRtStatus.pb.h"
#include "agent.h"

namespace Ui {
class ControlPanel;
}

class ControlPanel : public QMainWindow {
  Q_OBJECT

 public:
  explicit ControlPanel(QWidget *parent = nullptr, Agent *agent = nullptr);
  ~ControlPanel();
  void updateRtLabel();
  void updateAliveLabel();

  void closeEvent(QCloseEvent *) { emit windowsClosed(); }

  //cainiao::AgvRtStatusMessage::AgvRtStatus agv;

  int p40_X;
  int P40_y;


  //void resiveXY();

 signals:
  void windowsClosed();
  void putHeight(int height);
  void liftHeight(int height);
  void turnDirection(int direction);
  void moveForword(int grids);
  void moveBackword(int grids);

 private slots:
  // void on_pushButton_4_clicked(bool checked);

  void on_put_btn_clicked();

  void on_turn_btn_absolute_clicked();

  void on_move_btn_forword_clicked();

  void on_backward_btn_clicked();

  void zhuanquanquan();
  void zhuanquanquan1();
  void zhuanquanquan2();

  void on_lift_btn_clicked();

  void on_task_clicked();

  private:
  Ui::ControlPanel *ui;
  Agent *agent;
  cainiao::AgvRtStatusMessage::AgvRtStatus agv;
  QImage im_alive_on;
  QImage im_alive_off;
  bool alive{false};
  QImage im_rt_on;
  QImage im_rt_off;
  bool rt{false};
  QTimer *timer;

  QStandardItemModel *actionModel;
};

#endif  // CONTROL_PANEL_H
