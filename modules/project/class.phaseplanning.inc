<?php

  userelation("atkmanytoonerelation");
  useattrib("atknumberattribute");
  useattrib("project.spendhoursattribute");  

  class phaseplanning extends atkNode
  {
    function phaseplanning()
    {
      $this->atkNode("phaseplanning");
      $this->setSecurityAlias("project.phase");
      
      $this->add(new atkManyToOneRelation("phaseid", "project.phase", AF_HIDE|AF_PRIMARY));
      $this->add(new atkManyToOneRelation("personid", "employee.employee", AF_READONLY_EDIT|AF_PRIMARY));
      $this->add(new atkNumberAttribute("initial_planning", AF_READONLY_EDIT|AF_TOTAL));
      $this->add(new atkNumberAttribute("current_planning", AF_HIDE_ADD|AF_TOTAL));      
      $this->add(new spendHoursAttribute("spend_hours","phaseid",AF_TOTAL));
      
      $this->setTable("project_phaseplanning");
      $this->setOrder("personid.lastname");
    }
    
    function addDb(&$record, $exectrigger=true, $mode="add")
    {
      // This override makes sure that the current_planning is set equal to the number of initial_planning.
      $record["current_planning"] = $record["initial_planning"];
      return parent::addDb($record, $exectrigger, $mode);
    }
    
    function rowColor($record)
    {
      if($record["spend_hours"]>$record["current_planning"]) 
      {
        return COLOR_ERROR;
      }
      elseif($record["spend_hours"]>=$record["current_planning"]*0.9)
      {
        return COLOR_WARNING;
      }      
    }
        
  }

?>