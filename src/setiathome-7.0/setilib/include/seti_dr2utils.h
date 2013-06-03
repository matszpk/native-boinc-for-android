#include <vector>
#include <map>
#include "seti_dr2filter.h"

#define SOFTWARE_BLANKING_BIT 1
#define HARDWARE_BLANKING_BIT 2

// which HW data stream becomes which SW data stream is entirely
// determined by the device numbering of the EDT cards by the
// data recorder host OS.
typedef int channel_t;  // identifies a 2 bit SW data stream
typedef int input_t;    // identifies a 2 bit HW data stream

typedef std::pair<int,int> bmpol_t;

extern const int       blanking_input;
extern const short     blanking_bit;

extern std::map<telescope_id,channel_t> receiverid_to_channel;
extern std::map<channel_t,telescope_id> channel_to_receiverid;
extern std::map<bmpol_t,channel_t> beam_pol_to_channel;
extern std::map<input_t,channel_t> input_to_channel;

const size_t    DataSize         = 1024*1024;
const size_t    HeaderSize       = 4096;
const size_t    BufSize          = HeaderSize + DataSize;
const size_t    BufWords         = BufSize/2;
const size_t    DataWords        = DataSize/2;      // 16 bit data word size
const size_t    HeaderWords      = HeaderSize/2;
const size_t    dr2BufsPerBlock  = 128;      // number of contiguous data buffers of a given dsi
const size_t    dr2BlockSize     = BufSize*dr2BufsPerBlock;


typedef struct {
    double        Az;                 // should these be integers  ??
    double        Za;                 // gregorian position
    unsigned long Time;               // millisec past local midnight.  A time stamp for Az/Za
    time_t        SysTime;            // this and other SysTime's are local system time
} scram_agc_t;                      //   when these scam data were acquired by the recorder


typedef struct {
    int           AlfaFirstBias;      // bitmap
    int           AlfaSecondBias;     // bitmap
    float         AlfaMotorPosition;  // float is correct
    time_t        SysTime;
} scram_alfashm_t;


typedef struct {
    double        synI_freqHz_0;
    int           synI_ampDB_0;       // units .01 db (from AO source code)
    double        rfFreq;             // rf frequency hz. topo centric (from AO source code)
    double        if1FrqMhz;          // if1 freq Mhz (from AO source code)
    bool          alfaFb;             // 0 wb, 1-filter in (100 Mhz) (from AO source code)
    time_t        SysTime;
} scram_if1_t;


typedef struct {
    bool          useAlfa;            // 1 : alfa is on
    time_t        SysTime;
} scram_if2_t;

typedef struct {
    int           TurretEncoder;
    double        TurretDegrees;              // Encoder * (1./((4096.*210./5.0)/(360.)))
    double        TurretDegreesAlfa;          // needs to be hard coded
    double        TurretDegreesTolerance;     // needs to be hard coded
    time_t        SysTime;
} scram_tt_t;

typedef struct {                    // NOTE : these coords are *requested*, not actual (ie, do not use)
    double         Ra;                 // ra J2000 back converted from az,za (from AO source code)
    double         Dec;                // dec J2000 back converted from az,za (from AO source code)
    double         MJD;
    time_t         SysTime;
} scram_pnt_t;


class dataheader_t {
    public:
        int               header_size;
        int               data_size;
        char              name[36];
        int               dsi;            // data stream index
        int               channel;
        unsigned long     frameseq;
        unsigned long     dataseq;
        unsigned long     idlecount;      // dr2 buffer count of idle time
        int               missed;         // > 0 : there is a break from previous buffer to this one ?
        seti_time         data_time;      // time stamp : the final data sample in this block
        seti_time         coord_time;     // time stamp : ra/dec (ie, the time of Az/Za acquisition)
        seti_time         synth_time;     // time stamp : the setting of synth_freq
        double            ra;             // ra for the beam from which the data in this block came
        double            dec;            // dec for the beam from which the data in this block came
        long              synth_freq;     // Hz
        long              sky_freq;       // aka center frequency, Hz
        int               vgc[8];
        double            samplerate;
        scram_agc_t       scram_agc;
        scram_alfashm_t   scram_alfashm;
        scram_if1_t       scram_if1;
        scram_if2_t       scram_if2;
        scram_tt_t        scram_tt;
        scram_pnt_t       scram_pnt;
        bool              blanking_failed;    // true if we asked for data blanking and it failed, false otherwise

        dataheader_t();
        void populate_from_data (char *header);
        void populate_derived_values (channel_t this_channel);
        void print();

    private:
        int  parse_line(std::vector<char *> &words);
};

class dr2_compact_block_t {
    public:
        dataheader_t header;
        std::vector<complex<signed char> > data;
};

class dr2_block_t {
    public:
        dataheader_t header;
        std::vector<complex<float> > data;

//    dr2_block_t();
};

static blanking_filter<complex<float> > NULLF;
static blanking_filter<complex<signed char> > NULLC;


void read_header (char *header, dataheader_t &dataheader);
int  parse_header_line(std::vector<char *> &words, dataheader_t &dataheader);
int seti_GetDr2CommonWaveform(int fd, int dsi, ssize_t numsamples,
        std::vector<dr2_block_t> &dr2_block);
int seti_StructureDr2Data(int fd, telescope_id receiver, ssize_t numblocks,
        std::vector<dr2_block_t> &dr2_block,
        blanking_filter<complex<float> > filter=NULLF);
int seti_StructureDr2Data(int fd, int beam, int pol, ssize_t numblocks,
        std::vector<dr2_block_t> &dr2_block,
        blanking_filter<complex<float> > filter=NULLF);
int seti_StructureDr2Data(int fd, channel_t channel, ssize_t numsamples,
        std::vector<dr2_compact_block_t> &dr2_block,
        blanking_filter<complex<signed char> > filter=NULLC);
int seti_StructureDr2Data(int fd, telescope_id receiver, ssize_t numblocks,
        std::vector<dr2_compact_block_t> &dr2_block,
        blanking_filter<complex<signed char> > filter=NULLC);
int seti_StructureDr2Data(int fd, int beam, int pol, ssize_t numblocks,
        std::vector<dr2_compact_block_t> &dr2_block,
        blanking_filter<complex<signed char> > filter=NULLC);
bool seti_FindAdjacentDr2Buffer(int fd, dataheader_t &dataheader,
        off_t &adjacent_buffer_file_pos);
bool seti_PeekDr2Header(int fd, dataheader_t &this_header);
bool seti_Dr2DataTimesEqual(dataheader_t header1, dataheader_t header2);
int get_vgc_for_channel(channel_t channel, dataheader_t &header);

template <typename T>
class Dr2Channel {

        // IMPORTANT - multiple channels should be processed serially - never in parallel.
        // This because all Dr2Channel objects will presumably share the same file descriptor.

    private:
        int fd;
        off_t file_offset;
        channel_t channel;

        intel<unsigned short>   *data_buf;          // read data words into here
        size_t                  data_buf_i;         // index for data_buf
        intel<unsigned short>   *adjacent_buf;      // read adjacent data words into here
        intel<unsigned short>   *blanking_buf;      // will point to data_buf or adjacent_buf, depending on dsi
        size_t                  blanking_buf_i;     // index for blanking buf

        complex<T>              *dbuf;              // build up data samples here
        ssize_t                 dbuf_i;             // index for dbuf
        int                     *bbuf;              // build up blanking signal here
        ssize_t                 bbuf_i;             // index for bbuf

        int                     blanking_channel;
        int                     blanking_dsi;
        off_t                   this_header_pos;

        void                    acquire_blanking_signal(dataheader_t &dataheader, size_t numsamples, int dsi);

    public:
        Dr2Channel(int fdes, channel_t chan) : fd(fdes), channel(chan) {
            file_offset      = lseek(fd, 0, SEEK_CUR);
            data_buf_i       = 0;
            blanking_buf_i   = 0;
            blanking_channel = input_to_channel[blanking_input];
            blanking_dsi     = blanking_channel > 7 ? 0 : 1;
            data_buf         = (intel<unsigned short> *)calloc(BufSize,1);
            adjacent_buf     = (intel<unsigned short> *)calloc(BufSize,1);
            dbuf             = (complex<T> *)calloc(DataWords,sizeof(complex<T>));
            bbuf             = (int *)calloc(DataWords,sizeof(int));
        };
        void  SetChannel(channel_t chan)       {
            channel = chan;
        }
        int   GetChannel()               const {
            return(channel);
        }
        off_t GetFileOffset()            const {
            return(file_offset);
        }
        void  SetFileOffset(off_t offset)      {
            file_offset = offset;
        }
        int   GetNextData(size_t numsamples,
                std::vector<complex<T> > &c_samples,
                dataheader_t            *ret_dataheader,
                std::vector<int>         &blanking_signal,
                bool                     blanking
                         );
};

//----------------------------------------------------------------------------
template <typename T> int Dr2Channel<T>::GetNextData(
    size_t                  numsamples,
    std::vector<complex<T> > &c_samples,
    dataheader_t            *ret_dataheader,
    std::vector<int>         &blanking_signal,
    bool                     blanking=false
) {
//----------------------------------------------------------------------------

    dbuf_i = 0;

    ssize_t                 readbytes;
    unsigned short          dataword;
    long                    samplecount = 0;
    dataheader_t            dataheader;
    off_t                   seek_save;
    int                     dsi = channel > 7 ? 0 : 1;

    lseek(fd, file_offset, SEEK_SET);           // reset for this object
    if (file_offset == 0) {
        data_buf_i      = 0;
        blanking_buf_i  = 0;
    }

    if (numsamples != DataWords) {
        fprintf(stderr,
                "The number of samples requested (%d) is not equal to the number of samples in 1 buffer (%d).  This is not supported.", numsamples, DataWords);
        exit(1);
    }
    // This while() is a little bogus - currently we will always satisfy the condition on the first
    // time through, or break out of the loop. See the check immediatley above.
    while (samplecount < static_cast<long>(numsamples)) {

        // read the next buffer if need be
        if (data_buf_i == 0) {
            this_header_pos = lseek(fd, 0, SEEK_CUR);
            if ((readbytes = read(fd, data_buf, BufSize)) != BufSize) {
                break;
            }
            dataheader.populate_from_data((char *)data_buf);
            if (dataheader.dsi != dsi) {                    // correct data stream for desired channel?
                lseek(fd, dr2BlockSize-BufSize, SEEK_CUR);  // no - jump to next block
                continue;
            }
            data_buf_i = HeaderWords;                       // advance to first word past the header
        }

        // Get the first/next data from the dr2 buffer.  We exit this loop if *either*
        // we satisfy the numsamples request *or* we run out of data in this
        // buffer.  Currently, the latter condition will never be true as we require that
        // numsamples == the number of samples in one buffer.
        // It might be a bug that a request that demands the spanning
        // of multiple dr2 buffers would return less than numsamples.  Such a
        // return is usually treated as a failure by the caller.
        // data_buf_i indexes the data we just read into data_buf.
        // dbuf_i indexes the local buffers out of which we will do the block pushes.
        for (dbuf_i=0;
                data_buf_i < BufWords && samplecount < numsamples;
                data_buf_i += 1, dbuf_i +=1, samplecount += 1) {

            dataword = data_buf[data_buf_i];

            // isolate the desired channel
            dataword >>= (channel & 7)*2;

            // add the samples to our buffer
            dbuf[dbuf_i]=complex<T>(
                    (dataword & 2)-1,       // real, aka in-phase, aka i
                    ((dataword & 1)*2)-1    // imaginary, aka quadrature, aka q
                    );
        }

        // acquire blanking signal from this or the adjacent buffer
        // Adjacent means same dataseq, opposite dsi.
        if (blanking) {
            acquire_blanking_signal(dataheader, numsamples, dsi);
        }

        // here are the block pushes. dbuf_i indexes both dbuf and bbuf, as they are synchronized
        c_samples.insert(c_samples.end(),dbuf,dbuf+dbuf_i);                 // data
        if (blanking) {
            blanking_signal.insert(blanking_signal.end(),bbuf,bbuf+dbuf_i); // blanking signal
        }

        // if requested, return a fully populated header
        if (ret_dataheader) {
            dataheader.populate_derived_values(channel);
            *ret_dataheader = dataheader;       // a "shallow" assignment should be OK here
        }

        // if we exhausted the read buffer, we'll need to read the next buffer on the next call
        if (data_buf_i >= BufWords) {
            data_buf_i      = 0;
            blanking_buf_i  = 0;
        }
    } // end while (samplecount < numsamples)

    file_offset = lseek(fd, 0, SEEK_CUR);

    return(samplecount);
}

//----------------------------------------------------------------------------
template <typename T>
void Dr2Channel<T>::acquire_blanking_signal(dataheader_t &dataheader, size_t numsamples, int data_dsi) {
//----------------------------------------------------------------------------
#define DEBUG_acquire_blanking_signal

    ssize_t                 readbytes;
    unsigned short          dataword;
    off_t                   blanking_signal_file_pos;
    off_t                   seek_save;
    long                    blanking_samplecount = 0;

    bbuf_i = 0;

    if (blanking_buf_i == 0) {                              // blanking_buf_i != 0 means we already have our blanking buffer in memory
        if (blanking_dsi == data_dsi) {
            blanking_buf = data_buf;                        // blanking buf is data buf, trivial
        } else {
            blanking_buf = adjacent_buf;                    // blanking buf is adjacent buf, not trivial
            seek_save = lseek(fd, 0, SEEK_CUR);             // save data position
            lseek(fd, this_header_pos, SEEK_SET);           // back up to start of header
            if (seti_FindAdjacentDr2Buffer(fd, dataheader, blanking_signal_file_pos)) {
                lseek(fd, blanking_signal_file_pos, SEEK_SET);                      // go to adjacent position
                if ((readbytes = read(fd, blanking_buf, BufSize)) != BufSize) {     // acquire blanking signal
                    perror("Bad blanking buf read");
                    exit(1);
                }
            } else {
                dataheader.blanking_failed = true;
#ifdef DEBUG_acquire_blanking_signal
                fprintf(stderr, "Cannot find coincident blanking signal for dataseq %d - setting blanking signal to zeros\n", dataheader.dataseq);
#endif
            }
            lseek(fd, seek_save, SEEK_SET);                 // restore data position
        }  // end blanking_dsi != data_dsi
        blanking_buf_i = HeaderWords;                       // first word past the header
    }

    // this for loop should read a set of blanking signals coincedent with
    // the set of data samples that we read in the calling routine.
    // blanking_buf_i indexes the data read into or pointed to by blanking_buf.
    // bbuf_i indexes the local buffer out of which we will do the block push.
    for (bbuf_i=0;
            blanking_buf_i < BufWords && blanking_samplecount < numsamples;
            blanking_buf_i += 1, bbuf_i +=1, blanking_samplecount += 1
        ) {
        if (!dataheader.blanking_failed) {
            dataword = blanking_buf[blanking_buf_i];
            dataword >>= (blanking_channel & 7)*2;  // isolate the blanking channel
            if (dataword & blanking_bit) {
                bbuf[bbuf_i] = 1;     // blanking bit = true, so blank
            } else {
                bbuf[bbuf_i] = 0;     // blanking bit = false, so do not blank
            }
        } else {
            bbuf[bbuf_i] = 0;         // blanking bit acquisition failed, so do not blank
        }
    }
}

//----------------------------------------------------------------------------
template <typename T>
int seti_GetDr2Data(int                      fd,
        channel_t                channel,
        ssize_t                  numsamples,
        std::vector<complex<T> > &c_samples,
        dataheader_t            *ret_dataheader,
        std::vector<int>         &blanking_signal,
        bool                     blanking=false
                   ) {
//----------------------------------------------------------------------------
//  fd              device that contains tape data.
//  channel         the channel (0-15) from which you wish to extract.
//  numsamples      the number of samples you wish to extract.
//  c_samples       a std vector of type complex<T> onto which numsamples will be pushed.
//                  The samples will have values between -num_channels and
//                  +num_channels where num_channels is the number of active
//                  channels on that DSI. (6 for DSI0, 8 for DSI1)
//  ret_dataheader  pointer to a header object to receive the last header read.
//                  if NULL, then no header is "returned".
//  blanking_signal a binary (0 or 1) blanking signal coincident sample for sample
//                  with the data itself
//  blanking        Indicates whether or not to provide the blanking signal.  Defaults
//                  to off during development.
//  return val      the number of samples returned

    int retval;

    static Dr2Channel<T> dr2c(fd, channel);
    dr2c.SetChannel(channel);
    dr2c.SetFileOffset(lseek(fd, 0, SEEK_CUR));
    retval = dr2c.GetNextData(numsamples, c_samples, ret_dataheader,
            blanking_signal, blanking);
    return(retval);
}

