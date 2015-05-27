#ifndef VIDTRANSCODE_CV_ARM_CLIENT_INCLUDE_INTERNAL_IMAGE_H_
#define VIDTRANSCODE_CV_ARM_CLIENT_INCLUDE_INTERNAL_IMAGE_H_


#include <QtGlobal>
#include <QString>
#include <QDataStream>

namespace trik
{


class FormatID
{
  public:
    FormatID() : m_fmt() {}
    explicit FormatID(quint32 _fmt) : m_fmt(_fmt) {}

    operator bool() const { return m_fmt != 0; }

    quint32 id() const { return m_fmt; }

    QString toString() const
    {
      QString s;
      if (m_fmt == 0)
        s.append("<undefined>");
      else
        s.append((m_fmt    )&0xff)
         .append((m_fmt>>8 )&0xff)
         .append((m_fmt>>16)&0xff)
         .append((m_fmt>>24)&0xff);

      return s;
    }

  private:
    quint32 m_fmt;
};


struct ImageFormat
{
  ImageFormat() : m_width(), m_height(), m_format(), m_lineLength(), m_size() {}

  size_t   m_width;
  size_t   m_height;
  FormatID m_format;
  size_t   m_lineLength;
  size_t   m_size;
};

} // namespace trik


#endif // !VIDTRANSCODE_CV_ARM_CLIENT_INCLUDE_INTERNAL_IMAGE_H_
