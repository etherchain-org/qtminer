#include "QtMiner.h"

using namespace std;
using namespace dev;
using namespace dev::eth;

QtMiner::~QtMiner()
{
}

void QtMiner::run()
{
  
  QTimer::singleShot(0, this, [=](){
    this->m_rateTimer = startTimer(2000);
    this->m_pingTimer = startTimer(30000);
  });
  
  cnote << "Connecting to stratum server " << m_server.toStdString() << ":" << m_port;
  
  socket = new QTcpSocket(this);

  connect(socket, SIGNAL(connected()),this, SLOT(connected()));
  connect(socket, SIGNAL(disconnected()),this, SLOT(disconnected()));
  connect(socket, SIGNAL(readyRead()),this, SLOT(readyRead()));

  socket->connectToHost(m_server, m_port);
  
  if(!socket->waitForConnected(5000))
  {
    cnote << "Stratum connection error: " << socket->errorString().toStdString();
    emit finished();
  }  
  startMining();
}

void QtMiner::timerEvent(QTimerEvent* _e)
{
  if (_e->timerId() == m_rateTimer)
  {
    auto mp = f.miningProgress();
    f.resetMiningProgress();
    if (current)
      cnote << "Mining on PoWhash" << current.headerHash << ": " << mp;
    else
      cnote << "Waiting for work package...";
  }
  if (_e->timerId() == m_pingTimer) {
    cnote << "Pinging server...";
    sendGetWorkRequest();
  }
}

void QtMiner::sendGetWorkRequest()
{
  int requestId = requestCounter++;
  // cnote << requestId;
  requests[requestId] = "eth_getWork";
  QJsonObject obj;     
  obj["jsonrpc"] = "2.0";
  obj["method"] = "eth_getWork";
  QJsonArray params = { };
  obj["params"] = params;
  obj["id"] = requestId;
  QByteArray doc = QJsonDocument(obj).toJson(QJsonDocument::Compact);
  doc.append("\n");
  
  //qDebug() << doc;
  socket->write(doc);
}

void QtMiner::connected()
{
    cnote << "Connection to stratum server established!";
    
    int requestId = requestCounter++;   
    
    QJsonObject obj;    
    obj["jsonrpc"] = "2.0";
    obj["method"] = "eth_login";
    QJsonArray params = { m_user, m_pass };
    obj["params"] = params;
    obj["id"] = requestId;     
    requests[requestId] = "eth_login";    
    QByteArray doc = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    doc.append("\n");
    
    // qDebug() << doc;
    
    socket->write(doc);
}

void QtMiner::disconnected()
{
  cnote << "Connection to stratum server lost. Trying to reconnect!";
  this_thread::sleep_for(chrono::milliseconds(5000));
  
  while (true) {
    socket->connectToHost(m_server, m_port);

    if(!socket->waitForConnected(5000))
    {
      cnote << "Stratum connection error: " << socket->errorString().toStdString();
    } else {
      return;
    }
  }
}

void QtMiner::readyRead()
{
  //qDebug() << "reading...";

  // read the data from the socket
  QByteArray jsonData = socket->readAll();
    // qDebug() << jsonData;
  QJsonDocument document = QJsonDocument::fromJson(jsonData);
  QJsonObject object = document.object();
  QJsonValue jsonValue = object.value("id");
  double id = jsonValue.toDouble();
  
  // qDebug() << "Id is: " << id;
  QJsonValue result = object.value("result");
  
  if (result.isBool() && !result.toBool()) {
    cnote << "Stratum Server error:" << object.value("error").toString().toStdString();
    sendGetWorkRequest();
    return;
  }
  
  if (id == 0) {
    cnote << "Push: New work package received"; 
    QJsonArray workData = object["result"].toArray();
    processWorkPackage(workData);
  } else {
    QString method = requests[id];
    // qDebug() << method; 
    if (method == "eth_login") {
      requests.erase(id);
      cnote << "Login to stratum server successfull";
      sendGetWorkRequest();
    }    
    if (method == "eth_getWork") {
      requests.erase(id);
      cnote << "Work package received";        
      QJsonArray workData = object["result"].toArray();
      processWorkPackage(workData);
    } 
    if (method == "eth_submitWork") {
      requests.erase(id);
      cnote << "Share submitted to server!";        
      sendGetWorkRequest();
    }
  }
}

void QtMiner::processWorkPackage(QJsonArray workData) {
  h256 hh(workData[0].toString().toStdString());
  h256 newSeedHash(workData[1].toString().toStdString());
  // cnote << current.seedHash;
  // cout << newSeedHash << endl;
  if (current.seedHash != newSeedHash)
    cnote << "Grabbing DAG for" << newSeedHash;
  if (!(dag = EthashAux::full(newSeedHash, true, [&](unsigned _pc){ cout << "\rCreating DAG. " << _pc << "% done..." << flush; return 0; })))
    BOOST_THROW_EXCEPTION(DAGCreationFailure());
  if (m_precompute)
		EthashAux::computeFull(sha3(newSeedHash), true);
  if (hh != current.headerHash)
  {
    current.headerHash = hh;
    current.seedHash = newSeedHash;
    current.boundary = h256(fromHex(workData[2].toString().toStdString()), h256::AlignRight);
    cnote << "Got work package:";
    cnote << "  Header-hash:" << current.headerHash.hex();
    cnote << "  Seedhash:" << current.seedHash.hex();
    cnote << "  Target: " << h256(current.boundary).hex();
    f.setWork(current);
  }
}

void QtMiner::startMining() {
  
  if (m_minerType == "cpu")
			EthashCPUMiner::setNumInstances(m_miningThreads);
  else if (m_minerType == "opencl")
  {
    if (!EthashGPUMiner::configureGPU(
        m_localWorkSize,
        m_globalWorkSizeMultiplier,
        m_openclPlatform,
        m_openclDevice,
        m_clAllowCPU,
        m_extraGPUMemory,
        m_currentBlock,
	0,
	0
      ))
      exit(1);
    EthashGPUMiner::setNumInstances(m_miningThreads);
  }
    
  map<string, GenericFarm<EthashProofOfWork>::SealerDescriptor> sealers;
  sealers["cpu"] = GenericFarm<EthashProofOfWork>::SealerDescriptor{&EthashCPUMiner::instances, [](GenericMiner<EthashProofOfWork>::ConstructionInfo ci){ return new EthashCPUMiner(ci); }};
  
  sealers["opencl"] = GenericFarm<EthashProofOfWork>::SealerDescriptor{&EthashGPUMiner::instances, [](GenericMiner<EthashProofOfWork>::ConstructionInfo ci){ return new EthashGPUMiner(ci); }};
  
  std::string _m = m_minerType;
  unsigned _recheckPeriod = 500;
  
  (void)_m;
  (void)_recheckPeriod;
  
  f.setSealers(sealers);
  f.start(_m,0);
  f.onSolutionFound([&](EthashProofOfWork::Solution sol)
  {
    cnote << "Solution found; Submitting ...";
    cnote << "  Nonce:" << sol.nonce.hex();
    cnote << "  Mixhash:" << sol.mixHash.hex();
    cnote << "  Header-hash:" << current.headerHash.hex();
    cnote << "  Seedhash:" << current.seedHash.hex();
    cnote << "  Target: " << h256(current.boundary).hex();
    cnote << "  Ethash: " << h256(EthashAux::eval(current.seedHash, current.headerHash, sol.nonce).value).hex();
    emit solutionFound(QString::fromStdString("0x" + toString(sol.nonce)), QString::fromStdString("0x" + toString(current.headerHash)), QString::fromStdString("0x" + toString(sol.mixHash)));
    return true;
  });
}

void QtMiner::submitWork(QString nonce, QString headerHash, QString mixHash) {
  int requestId = requestCounter++; 
  QJsonObject obj;    
  obj["jsonrpc"] = "2.0";
  obj["method"] = "eth_submitWork";
  QJsonArray params = { nonce, headerHash, mixHash };
  obj["params"] = params;
  obj["id"] = requestId;     
  requests[requestId] = "eth_submitWork";    
  QByteArray doc = QJsonDocument(obj).toJson(QJsonDocument::Compact);
  doc.append("\n");
  
  socket->write(doc);
  current.reset();
}

















