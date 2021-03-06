#ifndef __SDR_DEMOD_HH__
#define __SDR_DEMOD_HH__

#include "node.hh"
#include "traits.hh"
#include "config.hh"
#include "combine.hh"
#include "logger.hh"
#include "math.hh"


namespace sdr {


/** Amplitude modulation (AM) demodulator from an I/Q signal.
 * @ingroup demods */
template <class Scalar>
class AMDemod
    : public Sink< std::complex<Scalar> >, public Source
{
public:
  /** Constructor. */
  AMDemod() : Sink< std::complex<Scalar> >(), Source()
  {
    // pass...
  }

  /** Destructor. */
  virtual ~AMDemod() {
    // free buffers
    _buffer.unref();
  }

  /** Configures the AM demod. */
  virtual void config(const Config &src_cfg) {
    // Requires type & buffer size
    if (!src_cfg.hasType() || !src_cfg.hasBufferSize()) { return; }
    // Check if buffer type matches template
    if (Config::typeId< std::complex<Scalar> >() != src_cfg.type()) {
      ConfigError err;
      err << "Can not configure AMDemod: Invalid type " << src_cfg.type()
          << ", expected " << Config::typeId< std::complex<Scalar> >();
      throw err;
    }

    // Unreference previous buffer
    _buffer.unref();
    // Allocate buffer
    _buffer = Buffer<Scalar>(src_cfg.bufferSize());

    LogMessage msg(LOG_DEBUG);
    msg << "Configure AMDemod: " << this << std::endl
        << " input type: " << Traits< std::complex<Scalar> >::scalarId << std::endl
        << " output type: " << Traits<Scalar>::scalarId << std::endl
        << " sample rate: " << src_cfg.sampleRate() << std::endl
        << " buffer size: " << src_cfg.bufferSize();
    Logger::get().log(msg);

    // Propergate config
    this->setConfig(Config(Config::typeId<Scalar>(), src_cfg.sampleRate(),
                           src_cfg.bufferSize(), src_cfg.numBuffers()));
  }

  /** Handles the I/Q input buffer. */
  virtual void process(const Buffer<std::complex<Scalar> > &buffer, bool allow_overwrite)
  {
    Buffer<Scalar> out_buffer;
    // If source allow to overwrite the buffer, use it otherwise rely on own buffer
    if (allow_overwrite) { out_buffer = Buffer<Scalar>(buffer); }
    else { out_buffer = _buffer; }

    // Perform demodulation
    for (size_t i=0; i<buffer.size(); i++) {
      out_buffer[i] = std::sqrt(buffer[i].real()*buffer[i].real() +
                                buffer[i].imag()*buffer[i].imag());
    }

    // If the source allowed to overwrite the buffer, this source will allow it too.
    // If this source used the internal buffer (_buffer), it allows to overwrite it anyway.
    this->send(out_buffer.head(buffer.size()), true);
  }

protected:
  /** The output buffer. */
  Buffer<Scalar> _buffer;
};


/** SSB upper side band (USB) demodulator from an I/Q signal.
 * @ingroup demods */
template <class Scalar>
class USBDemod
    : public Sink< std::complex<Scalar> >, public Source
{
public:
  /** The complex input scalar. */
  typedef std::complex<Scalar> CScalar;
  /** The real compute scalar. */
  typedef typename Traits<Scalar>::SScalar SScalar;

public:
  /** Constructor. */
  USBDemod() : Sink<CScalar>(), Source()
  {
    // pass...
  }

  /** Destructor. */
  virtual ~USBDemod() {
    _buffer.unref();
  }

  /** Configures the USB demodulator. */
  virtual void config(const Config &src_cfg) {
    // Requires type & buffer size
    if (!src_cfg.hasType() || !src_cfg.hasBufferSize()) { return; }
    // Check if buffer type matches template
    if (Config::typeId<CScalar>() != src_cfg.type()) {
      ConfigError err;
      err << "Can not configure USBDemod: Invalid type " << src_cfg.type()
          << ", expected " << Config::typeId<CScalar>();
      throw err;
    }

    // Unreference previous buffer
    _buffer.unref();
    // Allocate buffer
    _buffer =  Buffer<Scalar>(src_cfg.bufferSize());

    LogMessage msg(LOG_DEBUG);
    msg << "Configure USBDemod: " << this << std::endl
        << " input type: " << Traits< std::complex<Scalar> >::scalarId << std::endl
        << " output type: " << Traits<Scalar>::scalarId << std::endl
        << " sample rate: " << src_cfg.sampleRate() << std::endl
        << " buffer size: " << src_cfg.bufferSize();
    Logger::get().log(msg);

    // Propergate config
    this->setConfig(Config(Config::typeId<Scalar>(), src_cfg.sampleRate(),
                           src_cfg.bufferSize(), 1));
  }

  /** Performs the demodulation. */
  virtual void process(const Buffer<CScalar> &buffer, bool allow_overwrite) {
    if (allow_overwrite) {
      // Process in-place
      _process(buffer, Buffer<Scalar>(buffer));
    } else {
      // Store result in buffer
      _process(buffer, _buffer);
    }
  }

protected:
  /** The actual demodulation. */
  void _process(const Buffer< std::complex<Scalar> > &in, const Buffer< Scalar> &out) {
    for (size_t i=0; i<in.size(); i++) {
      out[i] = (SScalar(std::real(in[i])) + SScalar(std::imag(in[i])))/2;
    }
    this->send(out.head(in.size()));
  }

protected:
  /** The output buffer. */
  Buffer<Scalar> _buffer;
};



/** Demodulates FM from an I/Q signal.
 * This node only implements the demodulation of the signal, the needed post-filtering (deemphasize)
 * is implemented in a separate node, @c sdr::FMDeemph.
 * @ingroup demods */
template <class iScalar, class oScalar=iScalar>
class FMDemod: public Sink< std::complex<iScalar> >, public Source
{
public:
  /** The super scalar. */
  typedef typename Traits<iScalar>::SScalar SScalar;

public:
  /** Constructor. */
  FMDemod():
    Sink< std::complex<iScalar> >(), Source(), _shift(0), _can_overwrite(false)
  {
    _shift = 8*(sizeof(oScalar)-sizeof(iScalar));
  }

  /** Destructor. */
  virtual ~FMDemod() {
    _buffer.unref();
  }

  /** Configures the FM demodulator. */
  virtual void config(const Config &src_cfg) {
    // Requires type & buffer size
    if (!src_cfg.hasType() || !src_cfg.hasBufferSize()) { return; }
    // Check if buffer type matches template
    if (Config::typeId< std::complex<iScalar> >() != src_cfg.type()) {
      ConfigError err;
      err << "Can not configure FMDemod: Invalid type " << src_cfg.type()
          << ", expected " << Config::typeId< std::complex<iScalar> >();
      throw err;
    }
    // Unreference buffer if non-empty
    if (! _buffer.isEmpty()) { _buffer.unref(); }
    // Allocate buffer
    _buffer =  Buffer<oScalar>(src_cfg.bufferSize());
    // reset last value
    _last_value = 0;
    // Check if FM demod can be performed in-place
    _can_overwrite = (sizeof(std::complex<iScalar>) >= sizeof(oScalar));

    LogMessage msg(LOG_DEBUG);
    msg << "Configured FMDemod node: " << this << std::endl
        << " sample-rate: " << src_cfg.sampleRate() << std::endl
        << " in-type / out-type: " << src_cfg.type()
        << " / " << Config::typeId<oScalar>() << std::endl
        << " in-place: " << (_can_overwrite ? "true" : "false") << std::endl
        << " output scale: 2^" << _shift;
    Logger::get().log(msg);

    // Propergate config
    this->setConfig(Config(Config::typeId<oScalar>(), src_cfg.sampleRate(),
                           src_cfg.bufferSize(), 1));
  }

  /** Performs the FM demodulation. */
  virtual void process(const Buffer<std::complex<iScalar> > &buffer, bool allow_overwrite)
  {
    if (0 == buffer.size()) { return; }

    if (allow_overwrite && _can_overwrite) {
      _process(buffer, Buffer<oScalar>(buffer));
    } else {
      _process(buffer, _buffer);
    }
  }

protected:
  /** The actual demodulation. */
  void _process(const Buffer< std::complex<iScalar> > &in, const Buffer<oScalar> &out)
  {
    // Calc remaining values
    for (size_t i=1; i<in.size(); i++) {
      oScalar phi = fast_atan2<iScalar, oScalar>(in[i].real(), in[i].imag())/2;
      // dphi
      out[i] = (_last_value - phi);
      _last_value = phi;
    }

    // propergate result
    this->send(out.head(in.size()));
  }


protected:
  /** Output rescaling. */
  int _shift;
  /** The last angle. */
  oScalar _last_value;
  /** If true, in-place demodulation is poissible. */
  bool _can_overwrite;
  /** The output buffer, unused if demodulation is performed in-place. */
  Buffer<oScalar> _buffer;
};


/** A tiny node to de-emphasize the higher frequencies of a FM transmitted audio signal.
 * @ingroup filters */
template <class Scalar>
class FMDeemph: public Sink<Scalar>, public Source
{
public:
  /** Constructor. */
  FMDeemph(bool enabled=true)
    : Sink<Scalar>(), Source(), _enabled(enabled), _alpha(0), _avg(0), _buffer(0)
  {
    // pass...
  }

  /** Destructor. */
  virtual ~FMDeemph() {
    _buffer.unref();
  }

  /** Returns true if the filter node is enabled. */
  inline bool isEnabled() const { return _enabled; }

  /** Enable/Disable the filter node. */
  inline void enable(bool enabled) { _enabled = enabled; }

  /** Configures the node. */
  virtual void config(const Config &src_cfg) {
    // Requires type, sample rate & buffer size
    if (!src_cfg.hasType() || !src_cfg.hasSampleRate() || !src_cfg.hasBufferSize()) { return; }
    // Check if buffer type matches template
    if (Config::typeId<Scalar>() != src_cfg.type()) {
      ConfigError err;
      err << "Can not configure FMDeemph: Invalid type " << src_cfg.type()
          << ", expected " << Config::typeId<Scalar>();
      throw err;
    }
    // Determine filter constant alpha:
    _alpha = (int)round(
          1.0/( (1.0-exp(-1.0/(src_cfg.sampleRate() * 75e-6) )) ) );
    // Reset average:
    _avg = 0;
    // Unreference previous buffer
    _buffer.unref();
    // Allocate buffer:
    _buffer = Buffer<Scalar>(src_cfg.bufferSize());

    LogMessage msg(LOG_DEBUG);
    msg << "Configured FMDDeemph node: " << this << std::endl
        << " sample-rate: " << src_cfg.sampleRate() << std::endl
        << " type: " << src_cfg.type();
    Logger::get().log(msg);

    // Propergate config:
    this->setConfig(Config(src_cfg.type(), src_cfg.sampleRate(), src_cfg.bufferSize(), 1));
  }

  /** Dispatches in- or out-of-place filtering. */
  virtual void process(const Buffer<Scalar> &buffer, bool allow_overwrite)
  {
    // Skip if disabled:
    if (!_enabled) { this->send(buffer, allow_overwrite); return; }

    // Process in-place or not
    if (allow_overwrite) {
      _process(buffer, buffer);
      this->send(buffer, allow_overwrite);
    } else {
      _process(buffer, _buffer);
      this->send(_buffer.head(buffer.size()), false);
    }
  }

protected:
  /** Performs the actual filtering. */
  void _process(const Buffer<Scalar> &in, const Buffer<Scalar> &out) {
    for (size_t i=0; i<in.size(); i++) {
      // Update average:
      Scalar diff = in[i] - _avg;
      if (diff > 0) { _avg += (diff + _alpha/2) / _alpha; }
      else { _avg += (diff - _alpha/2) / _alpha; }
      // Store result
      out[i] = _avg;
    }
  }

protected:
  /** If true, the filter is enabled. If not, the node is a NOP. */
  bool _enabled;
  /** Filter constant. */
  int _alpha;
  /** Current averaged value. */
  Scalar _avg;
  /** The output buffer. */
  Buffer<Scalar> _buffer;
};

}

#endif // __SDR_DEMOD_HH__
