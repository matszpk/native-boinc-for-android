#!/usr/local/bin/php -q
<?php {
    // $Id: test_uc.php 1510 2003-06-17 01:36:47Z quarl $

    // This tests whether the most basic mechanisms are working
    // Also whether stderr output is reported correctly
    // Also tests if water levels are working correctly

    include_once("test_uc.inc");
    test_msg("standard upper_case application");

    $project = new ProjectUC;
    $project->start_servers_and_host();
    $project->validate_all_and_stop();

    test_done();
} ?>
