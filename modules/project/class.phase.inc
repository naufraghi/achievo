<?php

  useattrib("project.phasetemplateattribute");
  useattrib("atktextattribute");
  useattrib("atklistattribute");
  useattrib("atknumberattribute");
  useattrib("atkdateattribute");
  useattrib("utils.fuzzysearchattribute");  
  useattrib("project.spendhoursattribute");
  userelation("atkmanytoonerelation");
  userelation("atkmanyboolrelation");
  userelation("atkonetomanyrelation");
  
  
  /** 
   * Override of default atkManyBoolRelation because isEmpty depends not only on
   * setting of phase_activity but also of template.
   */
  class phaseManyBoolRelation extends atkManyBoolRelation 
  {
    function isEmpty($record)
    {    
      return (parent::isEmpty($record) && $record["template"]["id"]=="");
    }
  }
  
  class phase extends atkNode
  {
    function phase()
    {
      $this->atkNode("phase",NF_AUTOSELECT); // node() constructor is *not* called automatically!
  
      $this->add(new atkAttribute("id",AF_AUTOKEY));
      $this->add(new atkAttribute("name",AF_OBLIGATORY|AF_SEARCHABLE, 50));
      $this->add(new atkManyToOneRelation("projectid","project.project",AF_OBLIGATORY|AF_HIDE_LIST|AF_HIDE_EDIT));
  
      $this->add(new atkTextAttribute("description", TEXT_LARGE, AF_HIDE_LIST));
      $this->add(new atkListAttribute("status",array("active","nonactive")));
      
      $tmp = new phaseManyBoolRelation("phase_activity", "project.phase_activity", "project.activity", AF_HIDE_LIST|AF_OBLIGATORY);
      $tmp->m_localKey = "phaseid";
      $tmp->m_remoteKey = "activityid";
      $this->add($tmp);
  
      // template *must* come after phase_activity, because phase_activity does a delete of all
      // records on store, so template must be stored after phase_activity.
      $this->add(new phasetemplateAttribute("template","project.tpl_phase", AF_HIDE_LIST|AF_HIDE_EDIT));
      
      $this->add(new atkDateAttribute("startdate", "F d Y", "d F Y"));
      $this->add(new atkDateAttribute("enddate", "F d Y", "d F Y"));
  
      //$this->add(new atkNumberAttribute("max_phasetime"));
      $this->add(new atkNumberAttribute("initial_planning", AF_READONLY_EDIT));
      $this->add(new atkNumberAttribute("current_planning", AF_HIDE_ADD));
      $this->add(new spendHoursAttribute("spend_hours","id",AF_TOTAL));
      
      $this->add(new atkOneToManyRelation("phaseplanning", "project.phaseplanning", "phaseid", AF_HIDE_LIST));
      
      $this->add(new fuzzySearchAttribute("phaseplanning_add", "employee.employee", "storePhaseplanning", FSA_MULTISELECT, AF_HIDE_ADD|AF_BLANK_LABEL, 20));
  
      $this->setOrder("startdate, name");
      $this->setTable("phase","phase");
    }
  
    function descriptor_def()
    {
      return "[name]";
    }
  
    function initial_values()
    {
      return Array("status"=>"active");
    }
    
    function addDb(&$record, $exectrigger=true, $mode="add")
    {
      // This override makes sure that the current_planning is set equal to the number of initial_planning.
      $record["current_planning"] = $record["initial_planning"];
      return parent::addDb($record, $exectrigger, $mode);
    }
  
    function postDel($rec)
    {
      global $g_db;
      $delphase = $rec["id"];
  
      // delete dependencies for this phase./
      $query = "DELETE FROM dependency WHERE phaseid_row = $delphase OR phaseid_col = $delphase";
      $g_db->query($query);
    }  
  
    function action_select()
    {
      // In select mode, we place a filter, because we may only select active phases.
      $this->addFilter("status='active'");
  
      // Also, we hide the status field (we're showing only active phases,
      // so this column is not necessary.
      $this->m_attribList["status"]->m_flags|=AF_HIDE;
  
      // call parent method.
      return parent::action_select();
    }
    
    function phaseplanning_add_edit($record, $mode)
    {
      $org = $this->m_attribList["phaseplanning_add"]->edit($record, $mode);
      
      $hours = new atkAttribute("phaseplanning_initial");
      
      $dummy = array();
      return text("plan_employee", "phase")." ".$org." ".text("for")." ".$hours->edit($dummy)." ".text("hours");
      
    }
    
    function storePhaseplanning($rec, $people)
    {     
      $initial_planning = $this->m_postvars["phaseplanning_initial"];
      
      for ($i=0, $_i=count($people); $i<$_i; $i++)
      {        
        $this->_addPlanningRecord($rec["id"], $people[$i]["id"], $initial_planning);
      }
    }
    
    function _addPlanningRecord($phase_id, $person_id, $initial_planning)
    {
      global $g_db;
      
      // attendee may already exist.
      $recs = $g_db->getrows("SELECT count(*) AS cnt FROM project_phaseplanning WHERE phaseid = $phase_id AND personid = $person_id");
      if (!count($recs)||$recs[0]["cnt"]==0)
      {
        $g_db->query("INSERT INTO project_phaseplanning (phaseid, personid, current_planning, initial_planning)
                           VALUES ($phase_id, $person_id, $initial_planning, $initial_planning)");
      }
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