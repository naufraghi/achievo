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
        {
            registerNode("internal.timesummary", 
                         array("report", "any_user", "view_all", "any_project"));
            registerNode("internal.timebalance", 
                         array("report", "any_user", "view_all", "any_project"));
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