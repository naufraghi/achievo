<?php

  userelation("atkmanytoonerelation");
  useattrib("atknumberattribute");
  useattrib("atkdurationattribute");
  useattrib("project.spendhoursattribute");  

  class phaseplanning extends atkNode
  {
    function phaseplanning()
    {
      $this->atkNode("phaseplanning");
      $this->setSecurityAlias("project.phase");
      
      $this->add(new atkManyToOneRelation("phaseid", "project.phase", AF_HIDE|AF_PRIMARY));
      $this->add(new atkManyToOneRelation("personid", "employee.employee", AF_READONLY_EDIT|AF_PRIMARY));
      $this->add(new atkDurationAttribute("initial_planning",'','', AF_READONLY_EDIT|AF_TOTAL|AF_DURATION_STRING));
      $this->add(new atkDurationAttribute("current_planning",'','', AF_HIDE_ADD|AF_TOTAL|AF_DURATION_STRING));      
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
    
    function storePhases($phases)
    {
      global $g_db;
      
      foreach ($phases as $phaseid=>$persons)
      {
        foreach ($persons as $personid=>$current_planning)
        {
          if($phaseid!="" && $personid!="")
          {
            $recs = $g_db->getrows("SELECT count(*) as cnt
                                   FROM project_phaseplanning
                                   WHERE phaseid='$phaseid' AND personid='$personid'");
            $record = array("phaseid"=>$phaseid, "personid"=>$personid);
            $record["phaseid"] = $this->m_attribList["phaseid"]->db2value($record);
            $record["personid"] = $this->m_attribList["personid"]->db2value($record);
            $record["current_planning"] = $this->m_attribList["current_planning"]->_string2minutes($current_planning);
            if(!count($recs)|| $recs[0]["cnt"]==0)
            {
              $record["initial_planning"]=0;
              $this->addDb($record);
            }
            else
            {
              $record["atkprimkey"]="project_phaseplanning.phaseid='$phaseid' AND project_phaseplanning.personid='$personid'";
              $this->updateDb($record);
            }
            
          }
        }
      }
    }
        
  }

?>