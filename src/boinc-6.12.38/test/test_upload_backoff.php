#!/usr/local/bin/php -q
<?php {
    // $Id: test_upload_backoff.php 1526 2003-06-18 02:59:57Z quarl $

    $use_proxy_cgi = 1;

    include_once("test_uc.inc");
    test_msg("upload backoff");

    start_proxy('print "#$nconnections url=$url time=$time\n"; exit 1 if ($nconnections < 4); if_done_kill(); if_done_ping();');

    $project = new ProjectUC;

    // $pid = $host->run_asynch("-exit_when_idle");

    $project->start_servers_and_host();
    $project->validate_all_and_stop();

    test_done();
} ?>
