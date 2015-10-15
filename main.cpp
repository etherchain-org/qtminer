#include "QtMiner.h"
#include <QtWidgets/QApplication>
#include <QtCore>
#include <QCommandLineParser>

enum CommandLineParseResult
{
    CommandLineOk,
    CommandLineError,
    CommandLineVersionRequested,
    CommandLineHelpRequested
};

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
  
	QtMiner *app = new QtMiner(&a);
  
	QObject::connect(app, SIGNAL(finished()), &a, SLOT(quit()));
	QObject::connect(app, SIGNAL(solutionFound(QString, QString, QString)), app, SLOT(submitWork(QString, QString, QString)));
  
  QCoreApplication::setApplicationName("qtminer");
  QCoreApplication::setApplicationVersion("0.1.0");
  QCommandLineParser parser;
  parser.setApplicationDescription("Stratum enabled ethereum miner");
  parser.addHelpOption();
  parser.addVersionOption();
  QCommandLineOption serverOption({"s", "server"}, "Stratum server host (e.g. ethpool.org:3333)", "server", "ethpool.org:3333");
  parser.addOption(serverOption);
  QCommandLineOption userOption({"u", "user"}, "Username", "user");
  parser.addOption(userOption);
  QCommandLineOption passwordOption({"p", "pass"}, "Password", "pass", "x");
  parser.addOption(passwordOption);  
  QCommandLineOption gpuOption({"G", "gpu"}, "When mining use the GPU via OpenCL.");
  parser.addOption(gpuOption); 
  QCommandLineOption openClPlatformOption("opencl-platform", "When mining using -G/--opencl use OpenCL platform n (default: 0).", "n", "0");
  parser.addOption(openClPlatformOption);
  QCommandLineOption openClDeviceOption("opencl-device", "When mining using -G/--opencl use OpenCL device n (default: 0).", "n", "0");
  parser.addOption(openClDeviceOption);
  QCommandLineOption miningThreadsOption({"t", "mining-threads"}, "Limit number of CPU/GPU miners to n (default: use everything available on selected platform).", "n", "0");
  parser.addOption(miningThreadsOption);
  QCommandLineOption listDevicesOption("list-devices", "List the detected OpenCL devices and exit.");
  parser.addOption(listDevicesOption);
  QCommandLineOption currentBlockOption("current-block", "Let the miner know the current block number at configuration time. Will help determine DAG size and required GPU memory.", "n", "0");
  parser.addOption(currentBlockOption);
  QCommandLineOption extraGPUMemOption("cl-extragpu-mem", "Set the memory (in MB) you believe your GPU requires for stuff other than mining. Windows rendering e.t.c..", "n", "350000000");
  parser.addOption(extraGPUMemOption);
  QCommandLineOption clLocalWorkOption("cl-local-work", "Set the OpenCL local work size. Default is 64.", "n", "64");
  parser.addOption(clLocalWorkOption);
  QCommandLineOption clGlobalWorkOption("cl-global-work", "Set the OpenCL global work size as a multiple of the local work size. Default is 4096 * 64", "n", "0");
  parser.addOption(clGlobalWorkOption);
  QCommandLineOption msPerBatchOption("ms-per-batch", "Set the OpenCL target milliseconds per batch (global workgroup size). Default is 0. If 0 is given then no autoadjustment of global work size will happen.", "n", "0");
  parser.addOption(msPerBatchOption);
  QCommandLineOption noPrecomputeOption("no-precompute", "Don't precompute the next epoch's DAG.");
  parser.addOption(noPrecomputeOption);
  
  if (!parser.parse(QCoreApplication::arguments())) {
    parser.showHelp();
  }
  
  app->SetUser(parser.value(userOption));
  
  if (parser.isSet(serverOption)) {
    auto split = parser.value(serverOption).split(":");
    app->SetServer(split[0]);
    app->SetPort(split[1].toInt());
  }
  
  if (parser.isSet(gpuOption)) {
    app->EnableGPUMining();
  }
    
  if (parser.isSet(openClPlatformOption)) {
    app->SetOpenclPlatform(parser.value(openClPlatformOption).toInt());
  }
  if (parser.isSet(openClDeviceOption)) {
    app->SetOpenclDevice(parser.value(openClDeviceOption).toInt());
  }
  if (parser.isSet(miningThreadsOption)) {
    app->SetMiningThreads(parser.value(miningThreadsOption).toInt());
  }
  if (parser.isSet(currentBlockOption)) {
    app->SetCurrentBlock(parser.value(currentBlockOption).toInt());
  }
  if (parser.isSet(extraGPUMemOption)) {
    app->SetExtraGPUMemory(parser.value(extraGPUMemOption).toInt());
  }
  if (parser.isSet(clLocalWorkOption)) {
    app->SetLocalWorkSize(parser.value(clLocalWorkOption).toInt());
  }
  if (parser.isSet(clGlobalWorkOption)) {
    app->SetGlobalWorkSizeMultiplier(parser.value(clGlobalWorkOption).toInt());
  }
  if (parser.isSet(msPerBatchOption)) {
    app->SetMsPerBatch(parser.value(msPerBatchOption).toInt());
  }
  if (parser.isSet(noPrecomputeOption)) {
    app->DisablePrecompute();
  }  
  if (!parser.isSet(userOption)) {
    parser.showHelp();
  }
  
	QTimer::singleShot(0, app, SLOT(run()));
  
	return a.exec();
}
