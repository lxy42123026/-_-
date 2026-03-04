#include <signal.h>

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>

#include "QPointF"
#include "cmdline/cmdline.h"
#include "mainwindow.h"
#include "map_server.h"
#include "robot_proxy_server.h"
#include "spdlog/async.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

void sigint_handler(int sig) {
  if (sig == SIGINT) {
    QCoreApplication::quit();
  }
}

int main(int argc, char *argv[]) {
  signal(SIGINT, sigint_handler);
  // create a parser
  cmdline::parser cmd;
  cmd.add("gui", '\0', "start gui");
  cmd.parse_check(argc, argv);

  // configue spdlog
  spdlog::init_thread_pool(8192, 1);
  auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      "logs/rps_log.txt", 1024 * 1024 * 2, 5);
  std::vector<spdlog::sink_ptr> sinks{stdout_sink, rotating_sink};
  auto logger = std::make_shared<spdlog::async_logger>(
      "rps_logger", sinks.begin(), sinks.end(), spdlog::thread_pool(),
      spdlog::async_overflow_policy::block);
  logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%^%l%$](%P-%t): %v");
  spdlog::register_logger(logger);
  spdlog::set_default_logger(logger);
  spdlog::flush_every(std::chrono::seconds(5));

  //  QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

  //  QCoreApplication::setAttribute(Qt::AA_Use96Dpi);

  //  QPointF f;
  //  mapServer.getPointById(10351000, f);
  //  spdlog::info("test:{},{}", f.x(), f.y());
  // boolean flags are referred by calling exist() method.
  if (cmd.exist("gui")) {
    QApplication a(argc, argv);
    QObject::connect(&a, &QApplication::aboutToQuit, [&]() {
      spdlog::info("Exit rps.");
      spdlog::shutdown();
    });
    spdlog::info("Starting gui application...");
    MapServer mapServer;
    RobotProxyServer rps;
    rps.setMapServer(&mapServer);
    MainWindow w(nullptr, &rps);
    //    rps.setWindow(&w);
    w.show();
    return a.exec();
  }
  else {
    QCoreApplication a(argc, argv);
    QObject::connect(&a, &QCoreApplication::aboutToQuit, [&]() {
      spdlog::info("Exit rps.");
      spdlog::shutdown();
    });
    spdlog::info("Starting cli application...");
    MapServer mapServer;
    RobotProxyServer rps;

    rps.setMapServer(&mapServer);
    rps.startRPS("127.0.0.1", 7788, "127.0.0.1", 7789);
    return a.exec();
  }
}
