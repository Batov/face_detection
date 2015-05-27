#include "internal/codecengine.h"

#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QByteArray>

#include <libcodecengine-client/codecengine-client.h>



namespace trik
{

class CodecEngine::EngineControl
{
  public:
    EngineControl() : m_lock(), m_init(false) {}

    void init()
    {
      QMutexLocker locker(&m_lock);
      if (!m_init)
      {
        m_init = true;
        CERuntime_init();
      }
    }

    bool registerEngine(const QString& _serverName, const QString& _serverPath)
    {
      QByteArray serverName(_serverName.toLocal8Bit()); // pin temporary and make it char*
      QByteArray serverPath(_serverPath.toLocal8Bit()); // pin temporary and make it char*

      Engine_Desc ceDesc;
      Engine_initDesc(&ceDesc);
      ceDesc.name = serverName.data();
      ceDesc.remoteName = serverPath.data();

      Engine_Error ceError;
      if (1) // lock scope
      {
        QMutexLocker locker(&m_lock);
        ceError = Engine_add(&ceDesc);
      }

      if (ceError == Engine_EINUSE)
        qWarning() << "Engine_add(" << _serverName << "," << _serverPath << ") was already added";
      else if (ceError != Engine_EOK)
      {
        qWarning() << "Engine_add(" << _serverName << "," << _serverPath << ") failed:" << ceError;
        return false;
      }

      return true;
    }

  private:
    QMutex     m_lock;
    bool       m_init;
};




class CodecEngine::Handle
{
  public:
    Handle(const QString& _serverName)
     :m_engineHandle(0),
      m_serverHandle(0),
      m_opened(false)
    {
      QByteArray serverName(_serverName.toLocal8Bit()); // pin temporary, convert to char*

      Engine_Error ceError;
      m_engineHandle = Engine_open(serverName.data(), NULL, &ceError);
      if (!m_engineHandle)
      {
        qWarning() << "Engine_open(" << _serverName << ") failed:" << ceError;
        return;
      }

      m_serverHandle = Engine_getServer(m_engineHandle);
      if (!m_serverHandle)
        qWarning() << "Engine_getServer(" << _serverName << ") failed";

      m_opened = true;
    }

    ~Handle()
    {
      if (m_engineHandle)
        Engine_close(m_engineHandle);
    }

    bool opened() const
    {
      return m_opened;
    }

    Engine_Handle engineHandle() const
    {
      return m_engineHandle;
    }

    Server_Handle serverHandle() const
    {
      return m_serverHandle;
    }

  private:
    Engine_Handle m_engineHandle;
    Server_Handle m_serverHandle;
    bool          m_opened;

    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
};




CodecEngine::EngineControl CodecEngine::s_engineControl;

CodecEngine::CodecEngine(QObject* _parent)
 :QObject(_parent),
  m_handle(),
  m_serverName("dsp_server"),
  m_serverPath("dsp_server.bin")
{
  s_engineControl.init();
}

CodecEngine::~CodecEngine()
{
}


void
CodecEngine::setServerName(const QString& _name)
{
  m_serverName = _name;
}

void
CodecEngine::setServerPath(const QString& _path)
{
  m_serverPath = _path;
}

bool
CodecEngine::open()
{
  if (m_handle)
  {
    qWarning() << "CodecEngine already opened";
    return true;
  }

  s_engineControl.registerEngine(m_serverName, m_serverPath);

  m_handle.reset(new Handle(m_serverName));
  if (!m_handle->opened())
  {
    qWarning() << "CodecEngine open failed";
    m_handle.reset();
    return false;
  }

  emit opened();
  return true;
}

bool
CodecEngine::close()
{
  m_handle.reset();

  emit closed();
  return true;
}

void
CodecEngine::reportLoad()
{
  Server_Handle serverHandle = m_handle->serverHandle();
  if (!serverHandle)
  {
    qWarning() << "Cannot report DSP load, server handle is not available";
    return;
  }

  qDebug() << "DSP load" << Server_getCpuLoad(serverHandle);
}

} // namespace trik
