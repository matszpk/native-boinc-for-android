pro zones_to_xml, zones, file

    unit = 1
    openw, unit, file

    n = n_elements(zones)

    for i=0,n-1 do begin
        printf, unit, "<rfi_zone>"
        printf, unit, "    <min_time>", zones[i].min_time, "</min_time>"
        printf, unit, "    <max_time>", zones[i].max_time, "</max_time>"
        printf, unit, "    <central_detection_freq>", zones[i].CENTRAL_DETECTION_FREQ, "</central_detection_freq>"
        printf, unit, "    <detection_freq_width>", zones[i].detection_freq_width, "</detection_freq_width>"
        printf, unit, "    <central_period>", zones[i].central_period, "</central_period>"
        printf, unit, "    <period_width>", zones[i].period_width, "</period_width>"
        printf, unit, "    <fft_len_flags>", zones[i].fft_len_flags, "</fft_len_flags>"
        printf, unit, "    <signal_type_flags>", zones[i].signal_type_flags, "</signal_type_flags>"
        printf, unit, "</rfi_zone>"
    endfor

    close, unit

end
