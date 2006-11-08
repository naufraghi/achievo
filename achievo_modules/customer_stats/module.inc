<?php

  class mod_customer_stats extends achievoModule
  {
    function getNodes()
    {
      registerNode("customer_stats.simplehoursurvey",array("report","view_all","any_project"));
    }

    function pdaNodes()
    {
      return array(
      "hours_hoursurvey"=>dispatch_url("customer_stats.simplehoursurvey", "report"));
    }

    function getMenuItems()
    {
      $this->menuitem("customer_stats",
                      dispatch_url("customer_stats.simplehoursurvey", "report"),
                      "main",true,370);
      $this->menuitem("simplehoursurvey",
                      dispatch_url("customer_stats.simplehoursurvey", "report"),
                      "customer_stats");
    }
  }

?>