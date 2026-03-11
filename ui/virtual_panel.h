#ifndef VIRTUAL_PANEL_H
#define VIRTUAL_PANEL_H
#include <QMainWindow>
#include <spdlog/spdlog.h>

#include "virtual_robot.h"
//#include "control_panel.h"
#include "agent.h"


namespace Ui {
class VirtualPanel;
}

class VirtualPanel : public QMainWindow {
  Q_OBJECT

 public:

  explicit VirtualPanel(QWidget *parent = nullptr,
                        vir::VirtualRobot *virtualBot = nullptr,Agent *agent = nullptr);
  ~VirtualPanel();


  //void linkA(Agent *agent);

  cainiao::AgvRtStatusMessage::AgvRtStatus agv_1;
  void updateRtLabel();
  void updateAliveLabel();

  signals:
      //void linkRSCtoP40(int errorx);

 public slots:

  void set_textBrowser();

 private slots:

  void on_connectBtn_clicked(bool checked);

  void on_sendRgBtn_clicked();

  void on_sendStBtn_clicked();

  void on_stopSdBtn_clicked();




  void on_clearButton_clicked();

  void on_settingBtn_clicked();

  //void on_linkP40_clicked();

  //void on_pushButton_clicked();

  private:
  Ui::VirtualPanel *ui;
  vir::VirtualRobot *virtualBot;
  IMRState imr;

  Agent *agent;

  QTimer *timer1;//打印状态消息

  QImage im_alive_on;
  QImage im_alive_off;
  bool alive{false};
  QImage im_rt_on;
  QImage im_rt_off;
  bool rt{false};

};

#endif  // VIRTUAL_PANEL_H
