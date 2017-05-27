#pragma once
#include <libethcore/EthashGPUMiner.h>
#include <libethcore/EthashCPUMiner.h>
#include <libethash-cl/ethash_cl_miner.h>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QTimerEvent>


class QtMiner: public QObject
{
	Q_OBJECT

public:
	QtMiner(QObject *parent = 0) : QObject(parent) { }
	~QtMiner();
  
  void EnableCPUMining() { m_minerType = "cpu"; }
  void EnableGPUMining() { m_minerType = "opencl"; }
  void SetMiningThreads(unsigned miningThreads) { m_miningThreads = miningThreads; }
  void SetGlobalWorkSizeMultiplier(unsigned globalWorkSizeMultiplier) { m_globalWorkSizeMultiplier = globalWorkSizeMultiplier; }
  void SetLocalWorkSize(unsigned localWorkSize) { m_localWorkSize = localWorkSize; }
  void SetMsPerBatch(unsigned msPerBatch) { m_msPerBatch = msPerBatch; }
  void SetExtraGPUMemory(unsigned extraGPUMemory) { m_extraGPUMemory = extraGPUMemory; }
  void SetOpenclDevice(unsigned openclDevice) { m_openclDevice = openclDevice; m_miningThreads = 1; }
  void SetOpenclPlatform(unsigned openclPlatform) { m_openclPlatform = openclPlatform; }
  void SetCurrentBlock(uint64_t currentBlock) { m_currentBlock = currentBlock; }
  void SetUser(QString user) { m_user = user; }
  void SetPass(QString pass) { m_pass = pass; }
  void SetServer(QString server) { m_server = server; }
  void SetPort(unsigned port) { m_port = port; }
  void DisablePrecompute() { m_precompute = false; }
  
  void ListDevices() { dev::eth::EthashGPUMiner::listDevices(); emit finished(); }
  
	public slots:
    void run();
    void submitWork(QString nonce, QString headerHash, QString mixHash);

signals:
    void finished();
    void solutionFound(QString nonce, QString headerHash, QString mixHash);
private:
  std::string m_minerType = "cpu";
	unsigned m_openclPlatform = 0;
	unsigned m_openclDevice = 0;
	unsigned m_miningThreads = UINT_MAX;
  uint64_t m_currentBlock = 0;
	bool m_clAllowCPU = false;
	unsigned m_extraGPUMemory = 350000000;  
	bool m_precompute = true;
	QString m_user = "";
	QString m_pass = "x";
	QString m_server = "ethpool.org";
	unsigned m_port = 3333;
  
	unsigned m_globalWorkSizeMultiplier = ethash_cl_miner::c_defaultGlobalWorkSizeMultiplier;
	unsigned m_localWorkSize = ethash_cl_miner::c_defaultLocalWorkSize;
	unsigned m_msPerBatch = 0; //ethash_cl_miner::c_defaultMSPerBatch;
  
	// default value is 350MB of GPU memory for other stuff (windows system rendering, e.t.c.)
  
  struct NoWork {};
  
  dev::eth::EthashProofOfWork::WorkPackage current;
  dev::eth::GenericFarm<dev::eth::EthashProofOfWork> f;
  dev::eth::EthashAux::FullType dag;
  
  QTcpSocket *socket;
  std::map<double,QString> requests;
  
  int m_rateTimer;
  int m_pingTimer;
  int requestCounter = 1;
  void timerEvent(QTimerEvent*) override;
  void processWorkPackage(QJsonArray workData);
  
  void startMining();
  void sendGetWorkRequest();
private slots:  
    void connected();
    void disconnected();
    void readyRead();
};

