<?php
/*
 * Copyright 2007 Develer S.r.l. (http://www.develer.com/)
 * All rights reserved.
 *
 * $Id: $
 * Author: Matteo Bertini <naufraghi@develer.com>
 */

//TODO: sistemare i permessi... una volta capiti...
    class mod_internal extends atkModule
    {
        function getNodes()
        // Ricordarsi sempre di attivare il permesso nei security profiles dal web
        {
            registerNode("internal.timesummary",
                         array("report"));
            registerNode("internal.timebalance",
                         array("report", "export"));
        }
        function getDependencies($mode)
        {
            return array("timestats");
        }
        function getMenuItems()
        {
            $this->menuitem("internal",
                            dispatch_url("internal.timesummary", "report"),
                            "main", true, 370);
            $this->menuitem("time_summary",
                            dispatch_url("internal.timesummary", "report"),
                            "internal", true);
            $this->menuitem("time_balance",
                            dispatch_url("internal.timebalance", "report"),
                            "internal", true);
        }
    }
?>
