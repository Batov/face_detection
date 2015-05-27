#ifndef VIDTRANSCODE_CV_ARM_CLIENT_INCLUDE_INTERNAL_V4L2_H_
#define VIDTRANSCODE_CV_ARM_CLIENT_INCLUDE_INTERNAL_V4L2_H_

#include <QtGlobal>
#include <QObject>
#include <QString>
#include <QPointer>
#include <QScopedPointer>
#include <QSharedPointer>

#include "internal/image.h"


namespace trik
{

class V4L2;
class V4L2Frame;


class V4L2BufferMapper
{
  public:
    V4L2BufferMapper() : m_v4l2() {}
    virtual ~V4L2BufferMapper() { if (m_v4l2) detach(m_v4l2); }

    virtual size_t buffersCount() const = 0;

    virtual void attach(const QPointer<V4L2>& _v4l2);
    virtual void detach(const QPointer<V4L2>& _v4l2);

    virtual bool map() = 0;
    virtual bool unmap() = 0;

    virtual bool queueAll() = 0;
    virtual bool queue(size_t _index) = 0;
    virtual bool dequeue(const QSharedPointer<V4L2BufferMapper>& _bufferMapper, QSharedPointer<V4L2Frame>& _capturedFrame) = 0;

  protected:
    // friendly methods to access V4L2 protected methods by derived objects
    int v4l2_fd();
    bool v4l2_ioctl(int _request, void* _argp, bool _reportError = true, int* _errno = NULL);

  private:
    QPointer<V4L2> m_v4l2;

    V4L2BufferMapper(const V4L2BufferMapper&) = delete;
    V4L2BufferMapper& operator=(const V4L2BufferMapper&) = delete;
};


class V4L2BufferMapperMemoryMmap : public V4L2BufferMapper
{
  public:
    V4L2BufferMapperMemoryMmap(size_t _desiredBuffersCount);
    virtual ~V4L2BufferMapperMemoryMmap();

    virtual size_t buffersCount() const;

    virtual bool map();
    virtual bool unmap();

    virtual bool queueAll();
    virtual bool queue(size_t _index);
    virtual bool dequeue(const QSharedPointer<V4L2BufferMapper>& _bufferMapper, QSharedPointer<V4L2Frame>& _capturedFrame);

  private:
    size_t m_desiredBuffersCount;

    class Storage;
    QScopedPointer<Storage> m_storage;
};




class V4L2Frame
{
  public:
    V4L2Frame(const QSharedPointer<V4L2BufferMapper>& _bufferMapper, void* _ptr, size_t _size, size_t _index)
     :m_bufferMapper(_bufferMapper),
      m_ptr(_ptr),
      m_size(_size),
      m_index(_index)
    {
      Q_CHECK_PTR(m_bufferMapper);
    }

    ~V4L2Frame()
    {
      m_bufferMapper->queue(m_index);
    }

    void* ptr() const
    {
      return m_ptr;
    }

    size_t size() const
    {
      return m_size;
    }

    size_t index() const
    {
      return m_index;
    }
  private:
    QSharedPointer<V4L2BufferMapper> m_bufferMapper;
    void*          m_ptr;
    size_t         m_size;
    size_t         m_index;

    V4L2Frame(const V4L2Frame&) = delete;
    V4L2Frame& operator=(const V4L2Frame&) = delete;
};




class V4L2 : public QObject
{
  Q_OBJECT
  public:
    explicit V4L2(QObject* _parent);
    virtual ~V4L2();

  signals:
    void opened(const ImageFormat& _format);
    void closed();
    void started();
    void stopped();
    void frameCaptured(const QSharedPointer<V4L2Frame>& _capturedFrame);
    void fpsReported(qreal _fps);

  public slots:
    void setDevicePath(const QString& _path);
    void setFormat(const ImageFormat& _format);
    void setBufferMapper(const QSharedPointer<V4L2BufferMapper>& _bufferMapper);

    bool open();
    bool close();

    bool start();
    bool stop();

    void reportFps();

  protected:
    int fd() const;
    bool fd_ioctl(int _request, void* _argp, bool _reportError = true, int* _errno = NULL);

    void emitFrameCaptured(const QSharedPointer<V4L2Frame>& _capturedFrame);
    void emitFpsReported(qreal _fps);

  protected slots:
    void frameReadyIndication();

  private:
    friend class V4L2BufferMapper;

    struct Config
    {
      QString                          m_path;
      ImageFormat                      m_imageFormat;
      QSharedPointer<V4L2BufferMapper> m_bufferMapper;
    };
    Config m_config;

    class OpenCloseHandler;
    friend class OpenCloseHandler;
    QScopedPointer<OpenCloseHandler> m_openCloseHandler;

    class RunningHandler;
    friend class RunningHandler;
    QScopedPointer<RunningHandler> m_runningHandler;
};

} // namespace trik


#endif // !VIDTRANSCODE_CV_ARM_CLIENT_INCLUDE_INTERNAL_V4L2_H_
