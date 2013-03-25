bool is_rfi(common_signal_t &signal);

extern const long zone_rfi_mask;
extern const long staff_rfi_mask;       
extern const long volunteer_rfi_mask;  
extern const long birdie_rfi_mask;      
extern const long badval_rfi_mask;      
extern const long redundantsig_rfi_mask;
extern const long ignorewu_rfi_mask;    
extern const long ignorewug_rfi_mask;   
extern const long ignoretape_rfi_mask; 
extern const long null_rfi_mask;      


//___________________________________________________________________________________
template<typename T>
bool is_rfi(T typed_signal) {
//___________________________________________________________________________________
// This is an entry point for non-candidate based checks (eg, the sah assimilator).

    common_signal_t common_signal;
    
    common_signal.populate(typed_signal);
    return(is_rfi(common_signal));
}


