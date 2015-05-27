#include "internal/v4l2.h"

#include <QDebug>
#include <QByteArray>
#include <QScopedArrayPointer>
#include <QSocketNotifier>
#include <QTime>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/videodev2.h>
#include <libv4l2.h>


namespace trik
{

namespace // unnamed
{

class FdHandle
{
  public:
    explicit FdHandle(const QString& _path)
     :m_fd(-1)
    {
      const QByteArray& path(_path.toLocal8Bit()); // pin temporary, convert to char*

      m_fd = ::v4l2_open(path.data(), O_RDWR|O_NONBLOCK, 0);
      if (m_fd < 0)
      {
        m_fd = -1;
        qWarning() << "v4l2_open(" << _path << ") failed:" << errno;
        return;
      }
    }

    ~FdHandle()
    {
      if (m_fd != -1)
        ::v4l2_close(m_fd);
    }

    bool opened() const
    {
      return m_fd != -1;
    }

    int fd() const
    {
      return m_fd;
    }

    bool ioctl(int _request, void* _argp, bool _reportError, int* _errno)
    {
      if (!opened())
      {
        if (_errno)
          *_errno = ENOTCONN;
        if (_reportError)
          qWarning() << "v4l2_ioctl(" << _request << ") attempt on closed device";
        return false;
      }

      if (::v4l2_ioctl(m_fd, _request, _argp) == -1)
      {
        if (_errno)
          *_errno = errno;
        if (_reportError)
          qWarning() << "v4l2_ioctl(" << _request << ") failed:" << errno;
        return false;
      }

      if (_errno)
        *_errno = 0;

      return true;
    }

  private:
    int  m_fd;

    FdHandle(const FdHandle&) = delete;
    FdHandle& operator=(const FdHandle&) = delete;
};

} // namespace unnamed




int
V4L2BufferMapper::v4l2_fd()
{
  return m_v4l2->fd();
}

bool
V4L2BufferMapper::v4l2_ioctl(int _request, void* _argp, bool _reportError, int* _errno)
{
  return m_v4l2->fd_ioctl(_request, _argp, _reportError, _errno);
}

void
V4L2BufferMapper::attach(const QPointer<V4L2>& _v4l2)
{
  if (m_v4l2)
    qWarning() << "V4L2 buffer mapper already attached, dropping";
  m_v4l2 = _v4l2;
}

void
V4L2BufferMapper::detach(const QPointer<V4L2>& _v4l2)
{
  if (m_v4l2 != _v4l2)
  {
    qWarning() << "V4L2 buffer mapper detach attempt rejected";
    return;
  }
  m_v4l2 = QPointer<V4L2>();
}




class V4L2BufferMapperMemoryMmap::Storage
{
  public:
    explicit Storage(size_t _buffersCount) : m_buffersCount(_buffersCount), m_buffers(new MmapEntry[_buffersCount])
    {
      Q_CHECK_PTR(m_buffers);
    }

    ~Storage() = default;

    size_t count() const
    {
      return m_buffersCount;
    }

    bool getMapping(size_t _index, void*& _ptr, size_t& _size) const
    {
      if (_index >= m_buffersCount)
      {
        _ptr = MAP_FAILED;
        _size = 0;
        return false;
      }

      _ptr = m_buffers[_index].ptr();
      _size = m_buffers[_index].size();
      return true;
    }

    bool setMapping(size_t _index, int _fd, size_t _size, off_t _offset)
    {
      if (_index >= m_buffersCount)
        return false;

      return m_buffers[_index].map(_fd, _size, _offset);
    }

  private:
    class MmapEntry
    {
      public:
        explicit MmapEntry()
         :m_ptr(MAP_FAILED),
          m_size(0)
        {
        }

        ~MmapEntry()
        {
          if (m_ptr != MAP_FAILED)
            v4l2_munmap(m_ptr, m_size);
        }

        bool map(int _fd, size_t _size, off_t _offset)
        {
          m_ptr = v4l2_mmap(NULL, _size, PROT_READ|PROT_WRITE, MAP_SHARED, _fd, _offset);
          if (m_ptr == MAP_FAILED)
          {
            qWarning() << "v4l2_mmap failed:" << errno;
            return false;
          }
          m_size = _size;
          return true;
        }

        void* ptr() const
        {
          return m_ptr;
        }

        size_t size() const
        {
          return m_size;
        }

      private:
        void*  m_ptr;
        size_t m_size;
    };

    size_t m_buffersCount;
    QScopedArrayPointer<MmapEntry> m_buffers;
};




V4L2BufferMapperMemoryMmap::V4L2BufferMapperMemoryMmap(size_t _desiredBuffersCount)
 :V4L2BufferMapper(),
  m_desiredBuffersCount(_desiredBuffersCount),
  m_storage()
{
}

V4L2BufferMapperMemoryMmap::~V4L2BufferMapperMemoryMmap() = default;

size_t
V4L2BufferMapperMemoryMmap::buffersCount() const
{
  return m_storage ? m_storage->count() : 0;
}

bool
V4L2BufferMapperMemoryMmap::map()
{
  v4l2_requestbuffers requestBuffers = v4l2_requestbuffers();
  requestBuffers.count = m_desiredBuffersCount;
  requestBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  requestBuffers.memory = V4L2_MEMORY_MMAP;

  if (!v4l2_ioctl(VIDIOC_REQBUFS, &requestBuffers))
    return false;

  if (requestBuffers.count != m_desiredBuffersCount)
    qWarning() << "V4L2 demands" << requestBuffers.count << "buffers while" << m_desiredBuffersCount << "desired";

  m_storage.reset();
  QScopedPointer<Storage> storage(new Storage(requestBuffers.count));
  for (size_t idx = 0; idx < storage->count(); ++idx)
  {
    v4l2_buffer buffer = v4l2_buffer();
    buffer.index = idx;
    buffer.type = requestBuffers.type;
    buffer.memory = requestBuffers.memory;

    if (!v4l2_ioctl(VIDIOC_QUERYBUF, &buffer))
    {
      qWarning() << "V4L2 query buffer failed";
      return false;
    }

    storage->setMapping(idx, v4l2_fd(), buffer.length, buffer.m.offset);
  }

  m_storage.swap(storage);

  return true;
}

bool
V4L2BufferMapperMemoryMmap::unmap()
{
  m_storage.reset();

  return true;
}

bool
V4L2BufferMapperMemoryMmap::queueAll()
{
  for (size_t idx = 0; idx < m_storage->count(); ++idx)
  {
    if (!queue(idx))
      return false;
  }

  return true;
}

bool
V4L2BufferMapperMemoryMmap::queue(size_t _index)
{
  if (_index >= buffersCount())
    return false;

  v4l2_buffer buffer = v4l2_buffer();
  buffer.index  = _index;
  buffer.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buffer.memory = V4L2_MEMORY_MMAP;

  if (!v4l2_ioctl(VIDIOC_QBUF, &buffer))
  {
    qWarning() << "V4L2 queue buffer" << _index << "failed";
    return false;
  }

  return true;
}

bool
V4L2BufferMapperMemoryMmap::dequeue(const QSharedPointer<V4L2BufferMapper>& _bufferMapper, QSharedPointer<V4L2Frame>& _capturedFrame)
{
  v4l2_buffer buffer = v4l2_buffer();
  buffer.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buffer.memory = V4L2_MEMORY_MMAP;

  if (!v4l2_ioctl(VIDIOC_DQBUF, &buffer))
  {
    qWarning() << "V4L2 dequeue buffer failed";
    return false;
  }

  if (buffer.index >= buffersCount())
  {
    qWarning() << "V4L2 dequeue buffer" << buffer.index << "is out of range";
    return false;
  }

  void* ptr;
  size_t size;
  if (!m_storage->getMapping(buffer.index, ptr, size))
  {
    qWarning() << "V4L2 dequeue buffer" << buffer.index << "not mapped";
    return false;
  }

  _capturedFrame = QSharedPointer<V4L2Frame>(new V4L2Frame(_bufferMapper, ptr, size, buffer.index));

  return true;
}




class V4L2::OpenCloseHandler
{
  public:
    explicit OpenCloseHandler(const QPointer<V4L2>& _v4l2) :m_v4l2(_v4l2) {}
    ~OpenCloseHandler() { close(); }

    bool open(const V4L2::Config& _config)
    {
      m_fd.reset(new FdHandle(_config.m_path));
      if (!m_fd->opened())
      {
        m_fd.reset();
        return false;
      }

      if (!setFormat(_config.m_imageFormat))
        return false;

      return true;
    }

    void close()
    {
      m_fd.reset();
    }

    int fd() const
    {
      return m_fd ? m_fd->fd() : -1;
    }

    bool ioctl(int _request, void* _argp, bool _reportError, int* _errno)
    {
      if (!m_fd)
      {
        if (_errno)
          *_errno = ENOTCONN;
        if (_reportError)
          qWarning() << "v4l2_ioctl(" << _request << ") attempt on closed device";
        return false;
      }
      return m_fd->ioctl(_request, _argp, _reportError, _errno);
    }

    const ImageFormat& imageFormat() const
    {
      return m_imageFormat;
    }

  private:
    QPointer<V4L2> m_v4l2;
    QScopedPointer<FdHandle> m_fd;
    ImageFormat              m_imageFormat;


    void reportEmulatedFormats() const
    {
      for (size_t fmtIdx = 0; ; ++fmtIdx)
      {
        v4l2_fmtdesc fmtDesc;
        fmtDesc.index = fmtIdx;
        fmtDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        int err;
        if (!m_fd->ioctl(VIDIOC_ENUM_FMT, &fmtDesc, false, &err))
        {
          if (err != EINVAL)
            qWarning() << "v4l2_ioctl(VIDIOC_ENUM_FMT) failed:" << err;
          return;
        }

        if (fmtDesc.pixelformat == m_imageFormat.m_format.id())
        {
          if (fmtDesc.flags & V4L2_FMT_FLAG_EMULATED)
            qWarning() << "V4L2 format" << m_imageFormat.m_format.toString() << "is emulated, performance will be degraded";
          return;
        }
      }
    }

    bool setFormat(const ImageFormat& _imageFormat)
    {
      v4l2_format format = v4l2_format();
      format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      format.fmt.pix.width       = _imageFormat.m_width;
      format.fmt.pix.height      = _imageFormat.m_height;
      format.fmt.pix.pixelformat = _imageFormat.m_format.id();
      format.fmt.pix.field       = V4L2_FIELD_NONE;

      if (!m_fd->ioctl(VIDIOC_S_FMT, &format, true, NULL))
        return false;

      m_imageFormat.m_width      = format.fmt.pix.width;
      m_imageFormat.m_height     = format.fmt.pix.height;
      m_imageFormat.m_format     = FormatID(format.fmt.pix.pixelformat);
      m_imageFormat.m_lineLength = format.fmt.pix.bytesperline;
      m_imageFormat.m_size       = format.fmt.pix.sizeimage;


#warning Temporary, to be replaced by set fps
#if 1
      v4l2_streamparm streamparm = v4l2_streamparm();
      streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      if (!m_fd->ioctl(VIDIOC_S_PARM, &streamparm, true, NULL))
        return false;
      qDebug() << "Frame rate" << streamparm.parm.capture.timeperframe.numerator << "/" << streamparm.parm.capture.timeperframe.denominator;
#endif

      reportEmulatedFormats();

      return true;
    }

    OpenCloseHandler(const OpenCloseHandler&) = delete;
    OpenCloseHandler& operator=(const OpenCloseHandler&) = delete;
};




class V4L2::RunningHandler
{
  public:
    class BufferMapperWrapper
    {
      public:
        BufferMapperWrapper(const QPointer<V4L2>& _v4l2, const QSharedPointer<V4L2BufferMapper>& _bufferMapper)
         :m_v4l2(_v4l2),
          m_bufferMapper(_bufferMapper),
          m_opened(false)
        {
          Q_CHECK_PTR(m_v4l2);

          if (!m_bufferMapper)
            return;

          m_bufferMapper->attach(m_v4l2);

          if (!m_bufferMapper->map())
          {
            qWarning() << "V4L2 map failed";
            return;
          }

          if (!m_bufferMapper->queueAll())
          {
            qWarning() << "V4L2 queue all failed";
            return;
          }

          m_opened = true;
        }

        ~BufferMapperWrapper()
        {
          if (!m_bufferMapper)
            return;

          // do not de-queue buffers
          m_bufferMapper->unmap();
          m_bufferMapper->detach(m_v4l2);
        }

        bool opened() const
        {
          return m_opened;
        }

        bool captureFrame(QSharedPointer<V4L2Frame>& _capturedFrame)
        {
          if (!opened() || !m_bufferMapper)
            return false;

          return m_bufferMapper->dequeue(m_bufferMapper, _capturedFrame);
        }

      private:
        QPointer<V4L2> m_v4l2;
        QSharedPointer<V4L2BufferMapper> m_bufferMapper;
        bool m_opened;

        BufferMapperWrapper(const BufferMapperWrapper&) = delete;
        BufferMapperWrapper& operator=(const BufferMapperWrapper&) = delete;
    };

    class FpsCounter
    {
      public:
        FpsCounter()
         :m_timeout(),
          m_count()
        {
          m_timeout.start();
        }

        void frame()
        {
          ++m_count;
        }

        qreal calcFps()
        {
          int ms = m_timeout.restart();
          qreal fps = (ms > 0)
                    ? (static_cast<qreal>(m_count) * static_cast<qreal>(1000.0))/ static_cast<qreal>(ms)
                    :  static_cast<qreal>(m_count);
          m_count = 0;
          return fps;
        }

      private:
        QTime  m_timeout;
        size_t m_count;

        FpsCounter(const FpsCounter&) = delete;
        FpsCounter operator=(const FpsCounter&) = delete;
    };

    explicit RunningHandler(const QPointer<V4L2>& _v4l2) :m_v4l2(_v4l2) {}
    ~RunningHandler() { close(); }

    bool open(const V4L2::Config& _config)
    {
      m_bufferMapper.reset(new BufferMapperWrapper(m_v4l2, _config.m_bufferMapper));
      if (!m_bufferMapper->opened())
      {
        m_bufferMapper.reset();
        return false;
      }

      m_frameReadyNotifier.reset(new QSocketNotifier(m_v4l2->fd(), QSocketNotifier::Read, m_v4l2));
      connect(m_frameReadyNotifier.data(), SIGNAL(activated(int)), m_v4l2, SLOT(frameReadyIndication()));

      m_fpsCounter.reset(new FpsCounter());

      if (!startCapture())
      {
        qWarning() << "V4L2 queue buffers failed";
        return false;
      }

      return true;
    }

    void close()
    {
      stopCapture();
      m_fpsCounter.reset();
      m_frameReadyNotifier.reset();
      m_bufferMapper.reset();
    }

    void frameReadyIndication()
    {
      QSharedPointer<V4L2Frame> capturedFrame;
      if (!m_bufferMapper || !m_bufferMapper->captureFrame(capturedFrame))
      {
        qWarning() << "V4L2 cannot capture frame";
        return;
      }

      if (m_fpsCounter)
        m_fpsCounter->frame();

      m_v4l2->emitFrameCaptured(capturedFrame);
    }

    void reportFps() const
    {
      if (m_fpsCounter)
        m_v4l2->emitFpsReported(m_fpsCounter->calcFps());
    }

  private:
    QPointer<V4L2> m_v4l2;
    QScopedPointer<BufferMapperWrapper> m_bufferMapper;
    QScopedPointer<QSocketNotifier>     m_frameReadyNotifier;
    QScopedPointer<FpsCounter>          m_fpsCounter;

    bool startCapture()
    {
      v4l2_buf_type capture = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      if (!m_v4l2->fd_ioctl(VIDIOC_STREAMON, &capture))
      {
        qWarning() << "V4L2 stream on failed";
        return false;
      }

      return true;
    }

    bool stopCapture()
    {
      v4l2_buf_type capture = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      if (!m_v4l2->fd_ioctl(VIDIOC_STREAMOFF, &capture))
      {
        qWarning() << "V4L2 stream off failed";
        return false;
      }

      return true;
    }

    RunningHandler(const RunningHandler&) = delete;
    RunningHandler& operator=(const RunningHandler&) = delete;
};








V4L2::V4L2(QObject* _parent)
 :QObject(_parent),
  m_config(),
  m_openCloseHandler(),
  m_runningHandler()
{
  m_config.m_path = "/dev/video0";
  m_config.m_imageFormat.m_width = 640;
  m_config.m_imageFormat.m_height = 480;
  m_config.m_imageFormat.m_format = FormatID(V4L2_PIX_FMT_YUYV);
  m_config.m_bufferMapper = QSharedPointer<V4L2BufferMapper>(new V4L2BufferMapperMemoryMmap(2));
}

V4L2::~V4L2()
{
}

void
V4L2::setDevicePath(const QString& _path)
{
  m_config.m_path = _path;
}

void
V4L2::setFormat(const ImageFormat& _format)
{
  m_config.m_imageFormat = _format;
}

void
V4L2::setBufferMapper(const QSharedPointer<V4L2BufferMapper>& _bufferMapper)
{
  m_config.m_bufferMapper = _bufferMapper;
}

bool
V4L2::open()
{
  if (m_openCloseHandler)
    return true;

  m_openCloseHandler.reset(new OpenCloseHandler(this));
  if (!m_openCloseHandler->open(m_config))
  {
    m_openCloseHandler.reset();
    qWarning() << "V4L2 open failed";
    return false;
  }

#warning Temporary
  qDebug() << __func__ << m_openCloseHandler->imageFormat().m_width << "x" << m_openCloseHandler->imageFormat().m_height;

  emit opened(m_openCloseHandler->imageFormat());

  return true;
}

bool
V4L2::close()
{
  if (m_runningHandler) // just in case
    stop();

  if (!m_openCloseHandler)
    return true;

  m_openCloseHandler->close();
  m_openCloseHandler.reset();

  emit closed();

  return true;
}

bool
V4L2::start()
{
  if (!m_openCloseHandler)
  {
    if (!open() || !m_openCloseHandler)
      return false;
  }

  m_runningHandler.reset(new RunningHandler(this));
  if (!m_runningHandler->open(m_config))
  {
    m_runningHandler.reset();
    qWarning() << "V4L2 start failed";
    return false;
  }

  emit started();

  return true;
}

bool
V4L2::stop()
{
  if (!m_runningHandler)
    return true;

  m_runningHandler->close();
  m_runningHandler.reset();

  emit stopped();

  return true;
}

void
V4L2::frameReadyIndication()
{
  if (m_runningHandler)
    m_runningHandler->frameReadyIndication();
}

void
V4L2::reportFps()
{
  if (m_runningHandler)
    m_runningHandler->reportFps();
}

int
V4L2::fd() const
{
  if (!m_openCloseHandler)
    return -1;

  return m_openCloseHandler->fd();
}

void
V4L2::emitFrameCaptured(const QSharedPointer<V4L2Frame>& _capturedFrame)
{
#warning Temporary
  qDebug() << __func__ << "frame" << _capturedFrame->index() << _capturedFrame->ptr() << _capturedFrame->size();

  emit frameCaptured(_capturedFrame);
}

void
V4L2::emitFpsReported(qreal _fps)
{
#warning Temporary
  qDebug() << __func__ << _fps;

  emit fpsReported(_fps);
}

bool
V4L2::fd_ioctl(int _request, void* _argp, bool _reportError, int* _errno)
{
  if (!m_openCloseHandler)
  {
    if (_errno)
      *_errno = ENOTCONN;
    if (_reportError)
      qWarning() << "v4l2_ioctl(" << _request << ") attempt on closed device";
    return false;
  }

  return m_openCloseHandler->ioctl(_request, _argp, _reportError, _errno);
}


} // namespace trik
