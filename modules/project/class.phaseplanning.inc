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
    
    function postAdd($rec)
    {
      global $g_db;

      $phase_id = $rec["phaseid"]["id"];
      $person_id = $rec["personid"]["id"]; 
      // person may allready exist
      $recs = $g_db->getrows("SELECT count(*) as cnt 
                              FROM phase, project,project_person
                              WHERE phase.projectid = project.id
                                AND project.id = project_person.projectid
                                AND phase.id =$phase_id
                                AND project_person.personid = $person_id
                              GROUP BY project.id");
      if(!count($recs)|| $recs[0]["cnt"]==0)
      {
        $recs = $g_db->getrows("SELECT project.id FROM phase,project
                                WHERE phase.projectid = project.id
                                  AND phase.id = $phase_id");
        $project_id = $recs[0]["id"];
        // Add person to members, role will be empty
        $g_db->query("INSERT INTO project_person (projectid,personid)
                      values ($project_id,$person_id)");
      }
      
    }
        
  }

?>