// This file is automatically generated.  Do not edit
#ifndef _H_schema_master_H
#define _H_schema_master_H
extern const char *db_name;
extern int db_is_open;

#ifndef CLIENT
inline int db_open() { 
	  if (!db_is_open) db_is_open=sql_database(db_name);
	  return db_is_open; }
inline int db_close() { 
	  if (db_is_open) db_is_open=!sql_finish();
	  return !db_is_open; }
inline int db_change(const char *name) { 
	 if(strcmp(db_name, name) || !db_is_open) { 
		  db_close();
		  db_name=name;
		  db_open(); } return(db_is_open); }
#else
inline int db_open() { return (db_is_open=1); }
inline int db_close() { return !(db_is_open=0); }
inline int db_change() { return (db_is_open=1); }
#endif

class  coordinate_t  : public db_type<coordinate_t> {
  public:
	double  time;
	double  ra;
	double  dec;
	coordinate_t();
	coordinate_t(const coordinate_t &a);
	coordinate_t(const SQL_ROW &a);
	coordinate_t(const std::string &s,const char *tag="coordinate_t");
	coordinate_t &operator =(const coordinate_t &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="coordinate_t") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="coordinate_t");
  private:
};


class  chirp_parameter_t  : public db_type<chirp_parameter_t> {
  public:
	double  chirp_limit;
	long  fft_len_flags;
	chirp_parameter_t();
	chirp_parameter_t(const chirp_parameter_t &a);
	chirp_parameter_t(const SQL_ROW &a);
	chirp_parameter_t(const std::string &s,const char *tag="chirp_parameter_t");
	chirp_parameter_t &operator =(const chirp_parameter_t &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="chirp_parameter_t") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="chirp_parameter_t");
  private:
};


class  subband_description_t  : public db_type<subband_description_t> {
  public:
	long  number;
	double  center;
	double  base;
	double  sample_rate;
	subband_description_t();
	subband_description_t(const subband_description_t &a);
	subband_description_t(const SQL_ROW &a);
	subband_description_t(const std::string &s,const char *tag="subband_description_t");
	subband_description_t &operator =(const subband_description_t &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="subband_description_t") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="subband_description_t");
  private:
};


class  data_description_t  : public db_type<data_description_t> {
  public:
	double  start_ra;
	double  start_dec;
	double  end_ra;
	double  end_dec;
	double  true_angle_range;
	char  time_recorded[255];
	double  time_recorded_jd;
	long  nsamples;
	sqlblob<coordinate_t>  coords ;
	data_description_t();
	data_description_t(const data_description_t &a);
	data_description_t(const SQL_ROW &a);
	data_description_t(const std::string &s,const char *tag="data_description_t");
	data_description_t &operator =(const data_description_t &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="data_description_t") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="data_description_t");
  private:
};


class  receiver_config  : public db_table<receiver_config> {
  public:
	long  id;
	long  s4_id;
	char  name[255];
	double  beam_width;
	double  center_freq;
	double  latitude;
	double  longitude;
	double  elevation;
	double  diameter;
	double  az_orientation;
	sqlblob<float>  az_corr_coeff ;
	sqlblob<float>  zen_corr_coeff ;
	double  array_az_ellipse;
	double  array_za_ellipse;
	double  array_angle;
	long  min_vgc;
	receiver_config();
	receiver_config(const receiver_config &a);
	receiver_config(const SQL_ROW &a);
	receiver_config(const std::string &s,const char *tag="receiver_config");
	receiver_config &operator =(const receiver_config &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="receiver_config") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="receiver_config");
  private:
};


class  recorder_config  : public db_table<recorder_config> {
  public:
	long  id;
	char  name[64];
	long  bits_per_sample;
	double  sample_rate;
	long  beams;
	double  version;
	recorder_config();
	recorder_config(const recorder_config &a);
	recorder_config(const SQL_ROW &a);
	recorder_config(const std::string &s,const char *tag="recorder_config");
	recorder_config &operator =(const recorder_config &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="recorder_config") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="recorder_config");
  private:
};


class  splitter_config  : public db_table<splitter_config> {
  public:
	long  id;
	double  version;
	char  data_type[64];
	long  fft_len;
	long  ifft_len;
	char  filter[64];
	char  window[64];
	long  samples_per_wu;
	double  highpass;
	char  blanker_filter[64];
	splitter_config();
	splitter_config(const splitter_config &a);
	splitter_config(const SQL_ROW &a);
	splitter_config(const std::string &s,const char *tag="splitter_config");
	splitter_config &operator =(const splitter_config &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="splitter_config") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="splitter_config");
  private:
};


class  analysis_config  : public db_table<analysis_config> {
  public:
	long  id;
	double  spike_thresh;
	long  spikes_per_spectrum;
	double  gauss_null_chi_sq_thresh;
	double  gauss_chi_sq_thresh;
	double  gauss_power_thresh;
	double  gauss_peak_power_thresh;
	long  gauss_pot_length;
	double  pulse_thresh;
	double  pulse_display_thresh;
	long  pulse_max;
	long  pulse_min;
	long  pulse_fft_max;
	long  pulse_pot_length;
	double  triplet_thresh;
	long  triplet_max;
	long  triplet_min;
	long  triplet_pot_length;
	double  pot_overlap_factor;
	double  pot_t_offset;
	double  pot_min_slew;
	double  pot_max_slew;
	double  chirp_resolution;
	long  analysis_fft_lengths;
	long  bsmooth_boxcar_length;
	long  bsmooth_chunk_size;
	sqlblob<chirp_parameter_t>  chirps ;
	double  pulse_beams;
	long  max_signals;
	long  max_spikes;
	long  max_gaussians;
	long  max_pulses;
	long  max_triplets;
	long  keyuniq;
	double  credit_rate;
	analysis_config();
	analysis_config(const analysis_config &a);
	analysis_config(const SQL_ROW &a);
	analysis_config(const std::string &s,const char *tag="analysis_config");
	analysis_config &operator =(const analysis_config &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="analysis_config") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="analysis_config");
  private:
};


class  science_config  : public db_table<science_config> {
  public:
	long  id;
	long  active;
	char  qpix_scheme[16];
	long  qpix_nside;
	double  fpix_width;
	double  total_bandwidth;
	double  freq_uncertainty;
	double  fwhm_beamwidth;
	double  sky_disc_radius;
	double  observable_sky;
	long  epoch;
	double  bary_chirp_window;
	long  bary_freq_window;
	long  nonbary_freq_window;
	double  spike_obs_duration;
	double  spike_obs_interval;
	double  gauss_obs_duration;
	double  gauss_obs_interval;
	double  pulse_obs_duration;
	double  pulse_obs_interval;
	double  triplet_obs_duration;
	double  triplet_obs_interval;
	sqlint8_t  min_spike_id;
	sqlint8_t  min_gaussian_id;
	sqlint8_t  min_pulse_id;
	sqlint8_t  min_triplet_id;
	double  min_app_version;
	char  info_xml[255];
	science_config();
	science_config(const science_config &a);
	science_config(const SQL_ROW &a);
	science_config(const std::string &s,const char *tag="science_config");
	science_config &operator =(const science_config &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="science_config") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="science_config");
  private:
};


class  candidate_t  : public db_type<candidate_t> {
  public:
	long  type;
	sqlint8_t  id;
	long  num_obs;
	double  score;
	long  is_rfi;
	candidate_t();
	candidate_t(const candidate_t &a);
	candidate_t(const SQL_ROW &a);
	candidate_t(const std::string &s,const char *tag="candidate_t");
	candidate_t &operator =(const candidate_t &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="candidate_t") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="candidate_t");
  private:
};


class  meta_candidate  : public db_table<meta_candidate> {
  public:
	sqlint8_t  id;
	long  version;
	double  time_last_updated;
	long  num_spikes;
	long  num_spike_b_multiplets;
	double  best_spike_b_mp_score;
	long  num_spike_nb_multiplets;
	double  best_spike_nb_mp_score;
	sqlint8_t  spike_high_id;
	long  num_gaussians;
	long  num_gaussian_b_multiplets;
	double  best_gaussian_b_mp_score;
	long  num_gaussian_nb_multiplets;
	double  best_gaussian_nb_mp_score;
	sqlint8_t  gaussian_high_id;
	long  num_pulses;
	long  num_pulse_b_multiplets;
	double  best_pulse_b_mp_score;
	long  num_pulse_nb_multiplets;
	double  best_pulse_nb_mp_score;
	sqlint8_t  pulse_high_id;
	long  num_triplets;
	long  num_triplet_b_multiplets;
	double  best_triplet_b_mp_score;
	long  num_triplet_nb_multiplets;
	double  best_triplet_nb_mp_score;
	sqlint8_t  triplet_high_id;
	long  num_stars;
	double  best_star_score;
	double  meta_score;
	long  rfi_clean;
	long  state;
	meta_candidate();
	meta_candidate(const meta_candidate &a);
	meta_candidate(const SQL_ROW &a);
	meta_candidate(const std::string &s,const char *tag="meta_candidate");
	meta_candidate &operator =(const meta_candidate &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="meta_candidate") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="meta_candidate");
  private:
};


class  multiplet  : public db_table<multiplet> {
  public:
	long  id;
	long  version;
	long  signal_type;
	long  mp_type;
	long  qpix;
	double  freq_win;
	double  mean_ra;
	double  mean_decl;
	double  ra_stddev;
	double  decl_stddev;
	double  mean_angular_distance;
	double  angular_distance_stddev;
	double  mean_frequency;
	double  frequency_stddev;
	double  mean_chirp;
	double  chirp_stddev;
	double  mean_period;
	double  period_stddev;
	double  mean_snr;
	double  snr_stddev;
	double  mean_threshold;
	double  threshold_stddev;
	double  score;
	long  num_detections;
	sqlblob<sqlint8_t>  signal_ids ;
	multiplet();
	multiplet(const multiplet &a);
	multiplet(const SQL_ROW &a);
	multiplet(const std::string &s,const char *tag="multiplet");
	multiplet &operator =(const multiplet &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="multiplet") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="multiplet");
  private:
};


class  star  : public db_table<star> {
  public:
	long  id;
	char  object_type[16];
	char  catalog_name[64];
	long  catalog_number;
	char  object_name[64];
	double  ra;
	double  decl;
	long  qpix;
	double  v_mag;
	double  b_minus_v;
	double  parallax;
	char  stellar_type[32];
	long  planets;
	double  score;
	star();
	star(const star &a);
	star(const SQL_ROW &a);
	star(const std::string &s,const char *tag="star");
	star &operator =(const star &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="star") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="star");
  private:
};


class  candidate_count  : public db_table<candidate_count> {
  public:
	long  id;
	sqlint8_t  spikes;
	sqlint8_t  gaussians;
	sqlint8_t  pulses;
	sqlint8_t  triplets;
	sqlint8_t  spike_barycentric_multiplets;
	sqlint8_t  gaussian_barycentric_multiplets;
	sqlint8_t  pulse_barycentric_multiplets;
	sqlint8_t  triplet_barycentric_multiplets;
	sqlint8_t  spike_nonbarycentric_multiplets;
	sqlint8_t  gaussian_nonbarycentric_multiplets;
	sqlint8_t  pulse_nonbarycentric_multiplets;
	sqlint8_t  triplet_nonbarycentric_multiplets;
	sqlint8_t  stars;
	long  time_last_updated;
	candidate_count();
	candidate_count(const candidate_count &a);
	candidate_count(const SQL_ROW &a);
	candidate_count(const std::string &s,const char *tag="candidate_count");
	candidate_count &operator =(const candidate_count &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="candidate_count") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="candidate_count");
  private:
};


class  tape  : public db_table<tape> {
  public:
	long  id;
	char  name[20];
	double  start_time;
	double  last_block_time;
	long  last_block_done;
	long  missed;
	long  tape_quality;
	long  beam;
	tape();
	tape(const tape &a);
	tape(const SQL_ROW &a);
	tape(const std::string &s,const char *tag="tape");
	tape &operator =(const tape &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="tape") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="tape");
  private:
};


class  settings  : public db_table<settings> {
  public:
	long  id;
	long  active;
	db_reference<recorder_config,long> recorder_cfg;
	db_reference<splitter_config,long> splitter_cfg;
	db_reference<analysis_config,long> analysis_cfg;
	db_reference<receiver_config,long> receiver_cfg;
	settings();
	settings(const settings &a);
	settings(const SQL_ROW &a);
	settings(const std::string &s,const char *tag="settings");
	settings &operator =(const settings &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="settings") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="settings");
  private:
};


class  workunit_grp  : public db_table<workunit_grp> {
  public:
	long  id;
	db_reference<tape,long> tape_info;
	char  name[64];
	data_description_t data_desc;
	db_reference<receiver_config,long> receiver_cfg;
	db_reference<recorder_config,long> recorder_cfg;
	db_reference<splitter_config,long> splitter_cfg;
	db_reference<analysis_config,long> analysis_cfg;
	long  sb_id;
	workunit_grp();
	workunit_grp(const workunit_grp &a);
	workunit_grp(const SQL_ROW &a);
	workunit_grp(const std::string &s,const char *tag="workunit_grp");
	workunit_grp &operator =(const workunit_grp &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="workunit_grp") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="workunit_grp");
  private:
};


class  workunit_header  : public db_table<workunit_header> {
  public:
	sqlint8_t  id;
	char  name[64];
	db_reference<workunit_grp,long> group_info;
	subband_description_t subband_desc;
	sqlint8_t  sb_id;
	workunit_header();
	workunit_header(const workunit_header &a);
	workunit_header(const SQL_ROW &a);
	workunit_header(const std::string &s,const char *tag="workunit_header");
	workunit_header &operator =(const workunit_header &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="workunit_header") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="workunit_header");
  private:
};


typedef workunit_header workunit;
class  result  : public db_table<result> {
  public:
	sqlint8_t  id;
	sqlint8_t  boinc_result;
	db_reference<workunit_header,sqlint8_t> wuid;
	double  received;
	long  hostid;
	long  versionid;
	long  return_code;
	long  overflow;
	long  reserved;
	sqlint8_t  sb_id;
	result();
	result(const result &a);
	result(const SQL_ROW &a);
	result(const std::string &s,const char *tag="result");
	result &operator =(const result &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="result") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="result");
  private:
};


class  triplet  : public db_table<triplet> {
  public:
	sqlint8_t  id;
	db_reference<result,sqlint8_t> result_id;
	double  peak_power;
	double  mean_power;
	double  time;
	double  ra;
	double  decl;
	sqlint8_t  q_pix;
	double  freq;
	double  detection_freq;
	double  barycentric_freq;
	long  fft_len;
	double  chirp_rate;
	long  rfi_checked;
	long  rfi_found;
	long  reserved;
	double  period;
	triplet();
	triplet(const triplet &a);
	triplet(const SQL_ROW &a);
	triplet(const std::string &s,const char *tag="triplet");
	triplet &operator =(const triplet &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="triplet") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="triplet");
  private:
};


class  triplet_small  : public db_table<triplet_small> {
  public:
	sqlint8_t  id;
	db_reference<result,sqlint8_t> result_id;
	double  peak_power;
	double  mean_power;
	double  time;
	double  ra;
	double  decl;
	sqlint8_t  q_pix;
	double  freq;
	double  detection_freq;
	double  barycentric_freq;
	long  fft_len;
	double  chirp_rate;
	long  rfi_checked;
	long  rfi_found;
	long  reserved;
	double  period;
	triplet_small();
	triplet_small(const triplet_small &a);
	triplet_small(const SQL_ROW &a);
	triplet_small(const std::string &s,const char *tag="triplet_small");
	triplet_small &operator =(const triplet_small &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="triplet_small") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="triplet_small");
  private:
};


class  gaussian  : public db_table<gaussian> {
  public:
	sqlint8_t  id;
	db_reference<result,sqlint8_t> result_id;
	double  peak_power;
	double  mean_power;
	double  time;
	double  ra;
	double  decl;
	sqlint8_t  q_pix;
	double  freq;
	double  detection_freq;
	double  barycentric_freq;
	long  fft_len;
	double  chirp_rate;
	long  rfi_checked;
	long  rfi_found;
	long  reserved;
	double  sigma;
	double  chisqr;
	double  null_chisqr;
	double  score;
	double  max_power;
	sqlblob<unsigned char>  pot;
	gaussian();
	gaussian(const gaussian &a);
	gaussian(const SQL_ROW &a);
	gaussian(const std::string &s,const char *tag="gaussian");
	gaussian &operator =(const gaussian &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="gaussian") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="gaussian");
  private:
};


class  gaussian_small  : public db_table<gaussian_small> {
  public:
	sqlint8_t  id;
	db_reference<result,sqlint8_t> result_id;
	double  peak_power;
	double  mean_power;
	double  time;
	double  ra;
	double  decl;
	sqlint8_t  q_pix;
	double  freq;
	double  detection_freq;
	double  barycentric_freq;
	long  fft_len;
	double  chirp_rate;
	long  rfi_checked;
	long  rfi_found;
	long  reserved;
	double  sigma;
	double  chisqr;
	double  null_chisqr;
	double  score;
	double  max_power;
	gaussian_small();
	gaussian_small(const gaussian_small &a);
	gaussian_small(const SQL_ROW &a);
	gaussian_small(const std::string &s,const char *tag="gaussian_small");
	gaussian_small &operator =(const gaussian_small &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="gaussian_small") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="gaussian_small");
  private:
};


class  pulse  : public db_table<pulse> {
  public:
	sqlint8_t  id;
	db_reference<result,sqlint8_t> result_id;
	double  peak_power;
	double  mean_power;
	double  time;
	double  ra;
	double  decl;
	sqlint8_t  q_pix;
	double  freq;
	double  detection_freq;
	double  barycentric_freq;
	long  fft_len;
	double  chirp_rate;
	long  rfi_checked;
	long  rfi_found;
	long  reserved;
	double  period;
	double  snr;
	double  thresh;
	double  score;
	long  len_prof;
	sqlblob<unsigned char>  pot;
	pulse();
	pulse(const pulse &a);
	pulse(const SQL_ROW &a);
	pulse(const std::string &s,const char *tag="pulse");
	pulse &operator =(const pulse &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="pulse") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="pulse");
  private:
};


class  pulse_small  : public db_table<pulse_small> {
  public:
	sqlint8_t  id;
	db_reference<result,sqlint8_t> result_id;
	double  peak_power;
	double  mean_power;
	double  time;
	double  ra;
	double  decl;
	sqlint8_t  q_pix;
	double  freq;
	double  detection_freq;
	double  barycentric_freq;
	long  fft_len;
	double  chirp_rate;
	long  rfi_checked;
	long  rfi_found;
	long  reserved;
	double  period;
	double  snr;
	double  thresh;
	double  score;
	pulse_small();
	pulse_small(const pulse_small &a);
	pulse_small(const SQL_ROW &a);
	pulse_small(const std::string &s,const char *tag="pulse_small");
	pulse_small &operator =(const pulse_small &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="pulse_small") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="pulse_small");
  private:
};


class  sah_pointing  : public db_table<sah_pointing> {
  public:
	long  time_id;
	double  time;
	double  ra;
	double  dec;
	long  q_pix;
	double  angle_range;
	long  bad;
	sah_pointing();
	sah_pointing(const sah_pointing &a);
	sah_pointing(const SQL_ROW &a);
	sah_pointing(const std::string &s,const char *tag="sah_pointing");
	sah_pointing &operator =(const sah_pointing &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="sah_pointing") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="sah_pointing");
  private:
};


class  sky_map  : public db_table<sky_map> {
  public:
	sqlint8_t  npix;
	long  qpix;
	long  fpix;
	sqlint8_t  spike_max_id;
	sqlint8_t  gaussian_max_id;
	sqlint8_t  pulse_max_id;
	sqlint8_t  triplet_max_id;
	long  spike_count;
	long  gaussian_count;
	long  pulse_count;
	long  triplet_count;
	long  new_data;
	double  score;
	sky_map();
	sky_map(const sky_map &a);
	sky_map(const SQL_ROW &a);
	sky_map(const std::string &s,const char *tag="sky_map");
	sky_map &operator =(const sky_map &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="sky_map") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="sky_map");
  private:
};


class  hotpix  : public db_table<hotpix> {
  public:
	long  id;
	long  last_hit_time;
	hotpix();
	hotpix(const hotpix &a);
	hotpix(const SQL_ROW &a);
	hotpix(const std::string &s,const char *tag="hotpix");
	hotpix &operator =(const hotpix &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="hotpix") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="hotpix");
  private:
};


class  spike  : public db_table<spike> {
  public:
	sqlint8_t  id;
	db_reference<result,sqlint8_t> result_id;
	double  peak_power;
	double  mean_power;
	double  time;
	double  ra;
	double  decl;
	sqlint8_t  q_pix;
	double  freq;
	double  detection_freq;
	double  barycentric_freq;
	long  fft_len;
	double  chirp_rate;
	long  rfi_checked;
	long  rfi_found;
	long  reserved;
	spike();
	spike(const spike &a);
	spike(const SQL_ROW &a);
	spike(const std::string &s,const char *tag="spike");
	spike &operator =(const spike &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="spike") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="spike");
  private:
};


class  spike_small  : public db_table<spike_small> {
  public:
	sqlint8_t  id;
	db_reference<result,sqlint8_t> result_id;
	double  peak_power;
	double  mean_power;
	double  time;
	double  ra;
	double  decl;
	sqlint8_t  q_pix;
	double  freq;
	double  detection_freq;
	double  barycentric_freq;
	long  fft_len;
	double  chirp_rate;
	long  rfi_checked;
	long  rfi_found;
	long  reserved;
	spike_small();
	spike_small(const spike_small &a);
	spike_small(const SQL_ROW &a);
	spike_small(const std::string &s,const char *tag="spike_small");
	spike_small &operator =(const spike_small &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="spike_small") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="spike_small");
  private:
};


class  classic_versions  : public db_table<classic_versions> {
  public:
	long  id;
	long  ver_major;
	long  ver_minor;
	long  platformid;
	char  comment[254];
	char  filename[254];
	char  md5_cksum[254];
	char  sum_cksum[254];
	char  cksum_cksum[254];
	long  file_cksum;
	classic_versions();
	classic_versions(const classic_versions &a);
	classic_versions(const SQL_ROW &a);
	classic_versions(const std::string &s,const char *tag="classic_versions");
	classic_versions &operator =(const classic_versions &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="classic_versions") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="classic_versions");
  private:
};


class  classic_active_versions  : public db_table<classic_active_versions> {
  public:
	long  id;
	long  versionid;
	long  ver_major;
	long  ver_minor;
	classic_active_versions();
	classic_active_versions(const classic_active_versions &a);
	classic_active_versions(const SQL_ROW &a);
	classic_active_versions(const std::string &s,const char *tag="classic_active_versions");
	classic_active_versions &operator =(const classic_active_versions &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="classic_active_versions") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="classic_active_versions");
  private:
};


class  classic_active_versionids  : public db_table<classic_active_versionids> {
  public:
	long  id;
	long  versionid;
	classic_active_versionids();
	classic_active_versionids(const classic_active_versionids &a);
	classic_active_versionids(const SQL_ROW &a);
	classic_active_versionids(const std::string &s,const char *tag="classic_active_versionids");
	classic_active_versionids &operator =(const classic_active_versionids &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="classic_active_versionids") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="classic_active_versionids");
  private:
};


class  rfi_zone  : public db_table<rfi_zone> {
  public:
	long  id;
	long  min_receiver_s4id;
	long  max_receiver_s4id;
	long  min_splitter_config;
	long  max_splitter_config;
	long  min_analysis_config;
	long  max_analysis_config;
	long  min_tape_id;
	long  max_tape_id;
	sqlint8_t  min_workunit_id;
	sqlint8_t  max_workunit_id;
	sqlint8_t  min_result_id;
	sqlint8_t  max_result_id;
	double  min_time;
	double  max_time;
	double  central_baseband_freq;
	double  baseband_freq_width;
	double  central_detection_freq;
	double  detection_freq_width;
	double  central_period;
	double  period_width;
	long  fft_len_flags;
	long  signal_type_flags;
	double  ra;
	double  dec;
	double  angular_distance;
	rfi_zone();
	rfi_zone(const rfi_zone &a);
	rfi_zone(const SQL_ROW &a);
	rfi_zone(const std::string &s,const char *tag="rfi_zone");
	rfi_zone &operator =(const rfi_zone &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="rfi_zone") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="rfi_zone");
  private:
};


class  bad_data  : public db_table<bad_data> {
  public:
	char  name[20];
	long  beam;
	char  reason[255];
	bad_data();
	bad_data(const bad_data &a);
	bad_data(const SQL_ROW &a);
	bad_data(const std::string &s,const char *tag="bad_data");
	bad_data &operator =(const bad_data &a);
	std::string update_format() const;
	std::string insert_format() const;
	std::string select_format() const;
	std::string print(int full_subtables=0, int show_ids=1, int no_refs=0) const;
	std::string print_xml(int full_subtables=1, int show_ids=0, int no_refs=0,const char *tag="bad_data") const;
	void parse(const SQL_ROW &s);
	void parse(const std::string &s);
	void parse_xml(const std::string &s,const char *tag="bad_data");
  private:
};


#endif
