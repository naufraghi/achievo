<?php

include_once("achievotools.inc");
useattrib("project.projectphaseattrib");
useattrib("project.projectattrib");
useattrib("atkdateattribute");
useattrib("atklistattribute");
useattrib("atktextattribute");
useattrib("atkdurationattribute");
useattrib("atkdummyattribute");
useattrib("atktimeattribute");
userelation("atkmanytoonerelation");

function date_format($datestamp,$dateformat)
{
  if($dateformat!="")
  {
    return date($dateformat,$datestamp);
  }
  return date("m-d",$datestamp)." (".text(strtolower(date("D",$datestamp))).")";
}


/**
* Converts the starttime - endtime notation to a duration notation
*/
function convertHoursToDuration(&$record)
{
  $starttime  += $record["starttime"]["hours"]   *  60;
  $starttime  += $record["starttime"]["minutes"] *   1;
  $starttime  += $record["starttime"]["seconds"] /  60;

  $endtime    += $record["endtime"]["hours"]     *  60;
  $endtime    += $record["endtime"]["minutes"]   *   1;
  $endtime    += $record["endtime"]["seconds"]   /  60;

  $record["time"] = $endtime - $starttime;
}

function convertDurationToHours(&$record)
{
  if ($record['starttime'])
  {
    $rest = $record['time']%60;
    $record['time']               = $record['time']-$rest;
    $record['endtime']['hours']   = "".$record['starttime']['hours'] + $record['time']/60 ."";
    $record['time']               = $rest;

    $rest = $record['time']%1;
    $record['time']               = $record['time']-$rest;
    $record['endtime']['minutes'] = "".$record['starttime']['minutes']+$record['time']/1 ."";
    $record['time']               = $rest;

    $record['time']=round($record['time']*60);
    $record['endtime']['seconds']   = "".$record['starttime']['seconds']+$record['time'] ."";
  }
}

class to_date extends atkDateAttribute 
{
  function to_date()
  {
    $this->atkDateAttribute("todate", "F d Y","d F Y", 0, date("Ymd"), AF_NO_LABEL|AF_HIDE);
  }  
  
  function load() {}
  function store() {}  
  function addtoeditarray() {}
}

class to_time extends atkTimeAttribute 
{
  function to_time()
  {
    $this->atkTimeAttribute("endtime","0","23",array(0,15,30,45),"12", AF_OBLIGATORY);
  }
  function load() {}
  function store() {}  
  function display($record) {convertDurationToHours($record); return parent::display($record);}
}

class hours extends atkNode
{
  var $m_lock = "";
  var $m_viewdate = "";
  var $m_weekview = "";  
  var $m_user = "";
  var $m_insertmode = ""; // can be set to 'day' or 'multi'  

  function hours()
  {
    global $g_sessionManager,$ATK_VARS,$g_user;

    // weekview and viewdate are variables that we need to remember..
    $this->m_weekview = $g_sessionManager->pageVar("weekview");
    $this->m_viewdate = $g_sessionManager->pageVar("viewdate");
    $this->m_user = $g_sessionManager->pageVar("viewuser");
    if (is_array($this->m_viewdate)) // if set by the date jumper, viewdate is an array.
    {            
      $this->m_viewdate = sprintf("%04d-%02d-%02d", $this->m_viewdate["year"], $this->m_viewdate["month"],$this->m_viewdate["day"]);
    }
    if ($this->m_viewdate=="") $this->m_viewdate = strftime("%Y-%m-%d");
    if ($this->m_user == "") $this->m_user = $g_user["id"];
   
    $flags=NF_NO_VIEW|NF_TRACK_CHANGES;

    $this->atkNode("hours",$flags|NF_MRA); // node() constructor is *not* called automatically!

    // default view if none is set
    if (!isset($this->m_weekview)||$this->m_weekview=="")
    {
      if (atkconfig("timereg_defaultview")=="week") $this->m_weekview=1;
    }

    $this->add(new atkAttribute("id",AF_AUTOKEY));

    $this->add(new atkDateAttribute("activitydate","F d Y","d F Y", 0, date("Ymd"), AF_OBLIGATORY|AF_FORCE_LOAD|($this->m_weekview?0:AF_HIDE_LIST)));
    $this->add(new to_date());                

    // force_load flag is set for userid, since several features rely on the 
    // userid, even if it's not displayed on screen.
    $this->add(new atkManyToOneRelation("userid","employee.employee", AF_HIDE|AF_FORCE_LOAD|AF_OBLIGATORY));    

    $this->add(new atkProjectPhaseAttribute("phaseid",AF_OBLIGATORY|AF_JS_UPDATEACTIVITY));
    $this->add(new atkListAttribute("activityid",array("dummy","dummy","dummy","dummy","dummy"),array(0,0,0,0,0),AF_OBLIGATORY, 10));
    $this->add(new atkTextAttribute("remark", atkconfig("timereg_remark_lines", 1), 0));
    $this->addTime();
    
    $relation = new atkManyToOneRelation("workperiod","timereg.workperiod", AF_OBLIGATORY|AF_HIDE_LIST);
    $relation->setHideWhenEmpty(true);
    $this->add($relation);
    
    $this->add(new atkDateAttribute("entrydate",AF_HIDE|AF_OBLIGATORY));
    $this->add(new atkDummyAttribute("javascript", "seebelow", AF_NO_LABEL|AF_HIDE_LIST));

    $this->setOrder("hours.id"); // Sort by order of input..

    $this->setTable("hours","hours");
  }
  
  function addTime()
  {
    $durationflags = AF_TOTAL;
    
    if (!atkconfig("use_duration", true))
    {
      $this->add(new atkTimeAttribute("starttime","0","23",array(0,15,30,45),"12", AF_OBLIGATORY));
      $this->add(new to_time());
      $durationflags|=AF_HIDE_ADD|AF_HIDE_EDIT;
    }
    else 
    {
      $durationflags|=AF_OBLIGATORY;
    }
    
    $this->add(new atkDurationAttribute("time",atkconfig("timereg_resolution", "15m"), '10h',$durationflags));
  }
    
  function setLock()
  {
    $this->m_lock = $this->getLockType(strtolower($this->m_user),$this->m_viewdate);

    if ($this->m_lock != "")
    {
      // This week is locked..
      $flags |= NF_NO_ADD|NF_NO_DELETE|NF_NO_EDIT;
    }
  }
  
  function getInitialDate()
  {    
    if ($this->m_viewdate!="")
    {
       $initial_date = Array("year"=>substr($this->m_viewdate,0,4),
                             "month"=>substr($this->m_viewdate,5,2),
                             "day"=>substr($this->m_viewdate,8,2));
    }
    else
    {
      $initial_date = Array("year"=>date("Y"),
                            "month"=>date("m"),
                            "day"=>date("d"));
    }
    return $initial_date;
   
  }

  function initial_values()
  {
     return array("userid"=>array("id"=>$this->m_user),
                  "entrydate"=>Array("year"=>date("Y"),
                             "month"=>date("m"),
                             "day"=>date("d")),
                  "activitydate"=>$this->getInitialDate(),
                  "todate"=>$this->getInitialDate()
                  );
  }

  function activityid_display($rec)
  {
    // activityname is loaded in the phaseid join, so we get the info from there..
    return $rec["phaseid"]["activityname"];
  }

  function action_edit()
  {
    $recs = $this->selectDb($this->m_postvars["atkselector"], "hours.id", "", "", array("userid", "activitydate"));        
    
    $this->m_lock = $this->getLockType($recs[0]["userid"]["id"],
                      sprintf("%02d-%02d-%02d", $recs[0]["activitydate"]["year"], 
                                                $recs[0]["activitydate"]["month"],
                                                $recs[0]["activitydate"]["day"]));
    if ($this->m_lock=="")
    {
      $this->addFilter("hours.userid",strtolower($this->m_user));
      return parent::action_edit();
    }
    else
    {
      // Week is locked. We may not edit, only view.
      return $this->action_view();
    }
  }

  function javascript_edit($rec)
  {
    global $currentphase, $currentactivity;

    if (!isset($currentphase)||$currentphase=="") $currentphase=0;
    if (!isset($currentactivity)||$currentactivity=="") $currentactivity =0;
    $result = '<script language="javascript"> set_activities('.$currentphase.','.$currentactivity.'); </script>';

    return $result;
  }

  // override the admin action, because we have the weekview/dayview switch..
  function action_admin()
  {
    $this->setLock();

    $this->addFilter("hours.userid",$this->m_user);

    if ($this->m_weekview!=1)
    {
      if ($this->m_lock=="") $this->addPage();
      $this->addFilter("activitydate",$this->m_viewdate);
      $this->adminPage();
    }
    else
    {
      if (atkconfig("timereg_week_bookable")==true && $this->m_lock=="") $this->addPage();
      $this->weekview();
    }
  }

  function adminHeader()
  {
    if ($this->m_weekview!=1)
    {
      $res='<form name="dayview" method="post" action="dispatch.php">';
      $res.='<table border=0 cellpadding=0 cellspacing=0 width="100%" class="table">';
      $res.='<tr><td align="left" valign="top">';
      $viewdatestamp = adodb_mktime(0,0,0,substr($this->m_viewdate,5,2),substr($this->m_viewdate,8,2),substr($this->m_viewdate,0,4));

      // we substract 3601 seconds to account for Daylight Savings Time
      $yesterday = date('Y-m-d',$viewdatestamp-(86400-3601));

      // we add 3601 seconds to account for Daylight Savings Time
      $tomorrow = date('Y-m-d',$viewdatestamp+86400+3601);
      
      $today = date('Y-m-d');

      $res.= href('dispatch.php?viewuser='.$this->m_user.'&viewdate='.$yesterday, text("previousday"));
      if ($this->m_viewdate<date('Y-m-d')) 
      { 
        // If current is not today we can also move forward
        $res.='&nbsp;&nbsp;'.href('dispatch.php?viewuser='.$this->m_user.'&viewdate='.$tomorrow, text("nextday"));
        $res.='&nbsp;&nbsp;'.href('dispatch.php?viewuser='.$this->m_user.'&viewdate='.$today, text("today", "houradmin"));
      }
      $res.= '&nbsp;&nbsp;'.href('dispatch.php?viewuser='.$this->m_user.'&viewdate='.$this->m_viewdate.'&weekview=1', text("gotoweekview"));
      $res.='</td><td align="right" valign="top">';
      $res.=session_form();
      $res.=$this->getUserSelect().'&nbsp;';
      $res.=$this->getDateJumper();
      $res.='&nbsp;<input type="submit" value="'.text("goto").'">';
      $res.='</td></tr></table></form>';
      $res.='<b>'.text(strtolower(date("l", $viewdatestamp)))." ".text(strtolower(date("F", $viewdatestamp)))." ".date("d, Y", $viewdatestamp).'</b>'.$this->getLockIcon();
    }
    return $res;
  }

  function adminFooter()
  {
    if ($this->m_weekview!=1)
    {
      $balancenode = &getNode("timereg.overtime_balance");
      $balance = $balancenode->getBalance(date("Y-m-d", adodb_mktime(0,0,0,substr($this->m_viewdate,5,2),substr($this->m_viewdate,8,2),substr($this->m_viewdate,0,4))), $this->m_user);
      $res.='<table border=0 cellpadding=0 cellspacing=0 width="100%" class="table">';
      $res.='<tr><td align="left" valign="top">';
      $res.= href('dispatch.php?atknodetype=calendar.schedule&atkaction=admin&viewdate='.$this->m_viewdate, text("viewschedule"), SESSION_NESTED);
      $res.='</td><td align="right">'.text("current_balance").': '.time_format((60*$balance["balance"]),true).'&nbsp;</td></tr></table>';
      return $res;
    }
  }
  
  function getDateJumper()
  {
    $dummy_rec = array("viewdate"=>$this->getInitialDate());
    $datebox = new atkDateAttribute("viewdate","F d Y","d F Y",0, date("Ymd"),AF_OBLIGATORY);
    return $datebox->edit($dummy_rec);
  }
  
  function getUserSelect()
  {
    global $g_db;
    if ($this->allowed("any_user"))
    {
      $users = $g_db->getRows("SELECT id, lastname, firstname FROM person WHERE role = 'employee' AND status = 'active'");
      $res = '<select name="viewuser">';
      for ($i=0;$i<count($users);$i++)
      {
        $res .= '<option value="'.$users[$i]["id"].'" '.($this->m_user == $users[$i]["id"] ? 'selected' : '').'>'.$users[$i]["lastname"].', '.$users[$i]["firstname"];
      }
      $res .= '</select>'; 
    }  
    return $res;
  }

  function weekview()
  {
    global $g_layout;
    if ($this->m_viewdate!="")
    {
      $viewtime = adodb_mktime(12,12,0,substr($this->m_viewdate,5,2),substr($this->m_viewdate,8,2),substr($this->m_viewdate,0,4));
    }
    else
    {
      $this->m_viewdate = date("Y-m-d");
      $viewtime = time();
    }
    $g_layout->initGUI();
    $g_layout->ui_top(text('title_houradmin_weekview').' '.strftime('%V, %Y',$viewtime));

    $weekday = strftime("%w",$viewtime);
    if ($weekday==0) $weekday=7;

    for ($i=0; $i<=8;$i++)
    {
      $weekdates[$i]['date']=date("Y-m-d" ,$viewtime-(86400*($weekday-$i)));
      $weekdates[$i]['day']=text(strtolower(date("D" ,$viewtime-(86400*($weekday-$i)))));
      $weekdates[$i]['stamp']=$viewtime-(86400*($weekday-$i));
    }

    $week = Array();
    $projtotals = Array();
    $daytotals = Array();

    $data = $this->selectDb("activitydate >= '".$weekdates[1]['date']."' AND activitydate <= '".$weekdates[7]['date']."'","","",$this->m_listExcludes);

    for ($i=0;$i<count($data);$i++)
    {
      $rec = $data[$i];
      $key = $rec["phaseid"]["projname"]."-".$rec["phaseid"]["phasename"];
      $date = $rec["activitydate"]["year"]."-".$rec["activitydate"]["month"]."-".$rec["activitydate"]["day"];
      $time = $rec["time"];
      $week[$key][$date] += $time;
      $projtotals[$key] += $time;
      $daytotals[$date] += $time;
      $total += $time;
    }

    $userid = $this->m_user;

    // First a line with previous and next week links..
    $prevnextlinks='<form name="weekview" method="post" action="dispatch.php">';
    $prevnextlinks.=session_form();
    $prevnextlinks.='<table border=0 cellpadding=0 cellspacing=0 width="100%" class="table">';
    $prevnextlinks.='<tr><td valign="top" align="left">';
    $prevnextlinks.= ''.href('dispatch.php?viewuser='.$userid.'&weekview=1&viewdate='.$weekdates[0]['date'], text('previousweek'));
    if ($weekdates[8]['date'] <= date("Y-m-d"))
    {
      $prevnextlinks.='&nbsp;&nbsp;'.href('dispatch.php?viewuser='.$userid.'&weekview=1&viewdate='.$weekdates[8]['date'],text('nextweek'));
    }
    $prevnextlinks.= '&nbsp;&nbsp;'.href('dispatch.php?viewuser='.$userid.'&weekview=1&viewdate='.date("Y-m-d"), text('thisweek'));
    
    if ($this->m_lock=="individual" && $this->allowed("unlock"))
    {
      $prevnextlinks.='&nbsp;&nbsp;'.href('dispatch.php?atkaction=unlock&userid='.$userid.'&viewdate='.$this->m_viewdate,'<b>'.text("unlock").'</b>');
    }
    // users may only lock weeks that are not already locked. they must have the proper permission
    // and, if configured, they may only lock a week once they have booked all their hours.
    else if ($this->m_lock=="" && $this->allowed("lock") && (atkconfig("timereg_incompleteweeklock") || $this->weekComplete($userid, $this->m_viewdate)))
    {
      $prevnextlinks.='&nbsp;&nbsp;'.href('dispatch.php?atkaction=lock&userid='.$userid.'&viewdate='.$this->m_viewdate,'<b>'.text("lock").'</b>');
    }
    $prevnextlinks.='</td><td valign="top" align="right">';
    $prevnextlinks.=$this->getUserSelect().'&nbsp;';
    $prevnextlinks.=$this->getDateJumper();
    $prevnextlinks.='&nbsp;<input type="submit" value="'.text("goto").'">';
    $prevnextlinks.='</td></tr></table></form>';

    $prevnextlinks.='<b>'.text("week").' '.strftime('%V, %Y',$viewtime).'</b>'.$this->getLockIcon().'<br><br><br>';

    $g_layout->output($prevnextlinks);
    $g_layout->output($g_layout->data_top());
    $g_layout->output($g_layout->tr_top());

    $g_layout->td_datatitle(text('project').'/'.text('phase'));
    for ($i=1;$i<=7;$i++)
    {
      if ($weekdates[$i]['date'] <= date("Y-m-d"))
      {
        $url = 'dispatch.php?viewuser='.$userid.'&viewdate='.$weekdates[$i]['date']."&weekview=0";
        $g_layout->td_datatitle(href($url,text($weekdates[$i]['day'])).'<br>('.substr($weekdates[$i]['date'],5,2).'-'.substr($weekdates[$i]['date'],8,2).')','align="center"');
      }
      else
      {
        $g_layout->td_datatitle(text($weekdates[$i]['day']).'<br>('.substr($weekdates[$i]['date'],5,2).'-'.substr($weekdates[$i]['date'],8,2).')','align="center"');
      }

    }
    $g_layout->td_datatitle(text('total'));

    $g_layout->output($g_layout->tr_bottom());

    $row = 0;
    while (list($proj, $times)=each($week))
    {
      $g_layout->output($g_layout->tr_top((++$row)%2==0));
      $g_layout->td($proj);
      for ($i=1;$i<=7;$i++)
      {
        $g_layout->td(time_format($times[$weekdates[$i]['date']]), 'align="center"');
      }
      $g_layout->td(time_format($projtotals[$proj]), 'align="center"');
      $g_layout->output($g_layout->tr_bottom());
    }

    $g_layout->output($g_layout->tr_top());

    $g_layout->td_datatitle(text('total'));

    for ($i=1;$i<=7;$i++)
    {
      if ($daytotals[$weekdates[$i]['date']]>atkconfig("overtimethreshold"))
      {
        $color = "#FF0000";
      }
      else
      {
        $color = "#009900";
      }
      $g_layout->td_datatitle('<font color="'.$color.'">'.time_format($daytotals[$weekdates[$i]['date']]).'</font>','align="center"');
    }
    $g_layout->td_datatitle(time_format($total), 'align="center"');

    $g_layout->output($g_layout->tr_bottom());
    $g_layout->output($g_layout->data_bottom());
    $g_layout->ui_bottom();
  }

  function postAdd($rec)
  {
    // if we add a record on a date that is not the currently viewed date, we jump
    // to that date.
    // TODO this doesn't work anymore due to the fact that we move down in the session stack
    // where viewdate still has it's old value (do we need a way to alter the previous session?)
    $this->m_viewdate=$rec["activitydate"]["year"]."-".sprintf("%02d",$rec["activitydate"]["month"])."-".sprintf("%02d",$rec["activitydate"]["day"]);
    
    // update overtime balance
    $this->calculateNewBalance($rec, "add");
  }

  function postUpdate($rec)
  {
    // if we edit a record and set it to a date that is not the currently viewed date, we jump
    // to that date.
    // TODO this doesn't work anymore due to the fact that we move down in the session stack
    // where viewdate still has it's old value (do we need a way to alter the previous session?)
    $this->m_viewdate=$rec["activitydate"]["year"]."-".sprintf("%02d",$rec["activitydate"]["month"])."-".sprintf("%02d",$rec["activitydate"]["day"]);
    
    // update overtime balance
    $this->calculateNewBalance($rec, "edit");
  }
  
  function postDel($rec)
  {
    // update overtime balance
    $this->calculateNewBalance($rec, "delete");
  }
  
  function calculateNewBalance($rec, $mode)
  {
    $balancenode = &getNode("timereg.overtime_balance");
    $userid = $rec["userid"]["id"];
    $year  = $rec["activitydate"]["year"];
    $month = $rec["activitydate"]["month"];
    $activityday = $rec["activitydate"]["day"];
    $day = date("Y-m-d", adodb_mktime(0,0,0,$month,$activityday,$year));
    $yesterday = date("Y-m-d", adodb_mktime(0,0,0,$month,$activityday-1,$year));
    $balance = $balancenode->getBalance($day, $userid);
    
    // only update balance if balance was not set manually
    if ($balance["manual"] == 0)
    {
      if ($mode == "add")
      {
        // check if a record already exists for this day
        if ($balance["balance"] == text("not_inserted"))
        {
          // insert a new record with the balance of yesterday or 0
          $newbalance = $balancenode->getBalance($yesterday, $userid);
          if ($newbalance["balance"] == text("not_inserted")) $newbalance["balance"] = 0;
          $balancenode->addOvertimeBalance($userid, $day, $newbalance["balance"]);
        }
      }
      
      $percentage = $balancenode->getPercentage($rec["workperiod"]["id"]);
      $hours = ($rec["time"]/60);      
           
      if ($mode == "add") $newbalance = ($balance["balance"] + (($percentage/100) * $hours));
      elseif ($mode == "edit")
      {
        $oldpercentage = $balancenode->getPercentage($rec["atkorgrec"]["workperiod"]["id"]);        
        $oldhours = $rec["atkorgrec"]["time"]/60;
        $newbalance = (($balance["balance"] - (($oldpercentage/100) * $oldhours)) + (($percentage/100) * $hours));
      }
      elseif($mode == "delete") $newbalance = ($balance["balance"] - (($percentage/100) * $hours));
      
      $balancenode->saveOvertimeBalance($rec["userid"]["id"], $day, $newbalance);
    }  
    $manual = $balancenode->getLatestManualCorrection($rec["userid"]["id"], $yesterday);
    if ($manual != $yesterday)
    {
      $manualdate = date("Y-m-d", adodb_mktime(0,0,0,$manual["day"]["month"],$manual["day"]["day"],$manual["day"]["year"]));
    }
    else $manualdate = $yesterday;
    if ($manualdate < $day) $balancenode->deleteBalance($rec["userid"]["id"], $day);
  }

  // Check if this week may be booked..
  // returns: "" - not locked
  //          "individual" - locked for the user only
  //          "all"       - locked for everybody (can only be unlocked by administrator)
  function getLockType($userid, $date)
  {
    static $s_locks = array();
    global $g_db;
    
    $week = weekstamp(adodb_mktime(0,0,0,substr($date,5,2),
                                     substr($date,8,2),
                                     substr($date,0,4)));
    
    if ($s_locks[$userid][$week]=="")
    {      
      $query = "SELECT
                  week, userid
                FROM
                  hours_lock
                WHERE
                  (userid = '$userid' OR userid IS NULL OR userid='')
                AND
                  week = '$week'";
      $res = $g_db->getrows($query);
      
      if (count($res))
      {
        $all = false;
        for ($i=0;$i<count($res);$i++)
        {
          if (empty($res[$i]["userid"])) 
          {
            $s_locks[$userid][$week] = "all";
            $all = true;
          }
        }
    
        // apparently not locked for everybody.
        if (!$all)
        {
          $s_locks[$userid][$week] = "individual";
        }
      }
      else
      {
        $s_locks[$userid][$week]="none";
      }
    } 
        
    return ($s_locks[$userid][$week]=="none"?"":$s_locks[$userid][$week]);
  }

  function action_lock()
  {
    $viewdate = $this->m_postvars["viewdate"];
    $record["userid"]["id"] = $this->m_postvars["userid"];
    $record["week"] = weekstamp(adodb_mktime(0,0,0,substr($viewdate,5,2),
                                   substr($viewdate,8,2),
                                   substr($viewdate,0,4)));
    $locknode = &getNode("timereg.hours_lock");
    $locknode->addDb($record);
    $this->redirect();
  }

  function action_unlock()
  {
    global $g_db;

    $viewdate = $this->m_postvars["viewdate"];
    $userid = $this->m_postvars["userid"];
    $week = weekstamp(adodb_mktime(0,0,0,substr($viewdate,5,2),
                                   substr($viewdate,8,2),
                                   substr($viewdate,0,4)));
    $query = "DELETE FROM hours_lock WHERE week = '$week' AND userid = '$userid'";
    $g_db->query($query);
    $this->redirect();
  }

  /**
   * This function checks whether a week is 'compleet'. This means that the user
   * has booked the same ammount of hours (or more) as is specified in his contract.
   * Note: This is checked against the contract that was valid on the first day
   * of the week.
   */
  function weekComplete($userid, $date)
  {
    global $g_db;

    $start = startOfWeek($date);
    $end = endOfWeek($date);

    // first get the total of contract hours
    $sql = "SELECT sum(uc_hours*60) as contract FROM usercontract WHERE startdate <= '$start'
                AND (enddate > '$start' OR enddate IS NULL) AND userid = '$userid'";
    $result = $g_db->getRows($sql);
    
    // get the total of time already registered this week
    $query = "SELECT sum(time) as totaal
              FROM
                hours
              WHERE
                hours.userid = '$userid'
                AND activitydate between '$start' and '$end'";
    $res = $g_db->getrows($query);
    if (count($res)==0 || count($result)==0)
    {
      // either no contract in this period, or no hours booked at all..
      return false;
    }
    else
    {
      return ($result[0]["contract"] - $res[0]["totaal"]<=0);
    }
  }

  function activitydate_validate(&$rec)
  {
    // when inserting multiple days at once, we don't break on invalid days 
    // (we will just skip them later on).
    if ($rec["activitydate"]["multi"]!=1)
    {    
      if ($this->getLockType($this->m_user,sprintf("%04d-%02d-%02d",$rec["activitydate"]["year"],$rec["activitydate"]["month"],$rec["activitydate"]["day"]))!="")
      {
        triggerError($rec, "activitydate", "weeklocked");
      }
    }
  }
 /* 
  function phaseid_validate(&$rec, $mode)
  {    
    // Check if the user has rights to register time on this project.
    if ($mode=="add" && !$this->allowed("any_project"))
    {
      global $g_db;
      
      $user = getUser();     
      
      $cnt = $g_db->getrows("SELECT count(1) as cnt
                             FROM phase, project_person
                             WHERE phase.id = ".$rec["phaseid"]["phaseid"]." 
                               AND phase.projectid = project_person.projectid
                               AND project_person.personid = ".$user["id"]);
      if (!count($cnt)||$cnt[0]["cnt"]==0)
      {
        triggerError($rec, "phaseid", "error_timereg_members_only");
      }
    }
  }
*/
  /**
   * We override the validate method, so we can check if a remark is required
   * for the chosen activity. If it is, we add the AF_OBLIGATORY flag to the
   * remark before the validation.
   *
   * (First thought would be to create a remark_validate() function, but empty
   * values are not validated by atkNode's validate function. That's why we
   * need to override validate.)
   */
  function validate(&$record, $mode)
  {  
    $obj = getNode("project.activity");
    $act = $obj->selectDb("id=".$record["activityid"]);
    if (count($act)==1)
    {
      if ($act[0]["remarkrequired"]==1)
      {
        // Specification is required. So we set the obligatory flag.
        // The validation of this flag is done later on by the default 
        // ::validate() method of the base class.
        $this->m_attribList["remark"]->m_flags |= AF_OBLIGATORY;
      }
    }
    else // something is wrong with the chosen activity if we can't find
         // a record for it.
    {
      triggerError($record, "activityid", "error_obligatoryfield");
    }
    
    // when inserting multiple days at once, we don't break on invalid days 
    // (we will just skip them later on).    
    if ($record["activitydate"]["multi"]!=1)
    {
      $activitydate = sprintf("%04d-%02d-%02d", $record["activitydate"]["year"], $record["activitydate"]["month"], $record["activitydate"]["day"]);
      $new_minutes = $record["time"];

      if ($this->exceed_hrs_in_day($record["userid"]["id"], $activitydate, $new_minutes, $record["id"]))
      {
        triggerError($record, "time", "error_hoursindayexceeded");
      }
    }
    else // when inserting multiple days at once, we must verify if end is not smaller than start.
    {
      $start = date("Y-m-d", adodb_mktime(12,0,0,$record["activitydate"]["month"],$record["activitydate"]["day"],$record["activitydate"]["year"]));
      $todate = $this->m_postvars["todate"]; // todate is not part of $record, because the todate attrib isn't really a part of the node.      
      $end = date("Y-m-d", adodb_mktime(12,0,0,$todate["month"],$todate["day"],$todate["year"]));
      atkdebug("validating $start vs $end");
      if ($start>$end)
      {
        triggerError($record, "activitydate", "error_startbiggerthanend");
      }
    }
    // continue with original validation
    atkNode::validate($record, $mode);
  }

 /**
   * This method is called by the hoursurvey to get custom search fields.
   * Derived classes (for example in overloaders) can override this method and
   * Add their own custom search fields.
   */

  function getSelectionItems()
  {
    return array();
  }

  function getHourSurveyColumns()
  {
    return array("activitydate","userid","phaseid","activityid","remark","time");
  }

  
  function exceed_hrs_in_day($userid, $date, $new_minutes, $recordid="")
  {
    global $g_db;

    $query = "SELECT sum(time) + $new_minutes minutes_in_day
              FROM hours 
              WHERE 
                userid = '$userid' 
                AND activitydate = '$date'";
                
    // If we are editing a record, than the old value of the current record
    // must not be counted, but the new value (which was already added as
    // $new_minutes).
    if ($recordid!="") $query.= " AND id <> $recordid";
    $res = $g_db->getrows($query);
    if (count($res)==0)
    {
      // no records for $date
      return false;
    }
    else
    {
      return ($res[0]["minutes_in_day"] > 24*60 );
    }
  }    
  
  function addPage($record="")
  {
    global $g_sessionManager;
   // Insert mode can be "" (normal) or "multi" (multiple days at once).
    $this->m_insertmode = $g_sessionManager->stackVar("insertmode");    
    if ($this->m_insertmode=="multi")
    {
      // When in multidate mode, add an extra attribute for filtering
      // what days we want to filter.      
      $this->add(new atkDummyAttribute("dayfilter", "", AF_HIDE_EDIT|AF_HIDE_LIST));
    }
   
    return parent::addPage($record);
  }
  
  function activitydate_edit($record, $prefix="", $mode="")
  {  
    if ($mode=="add")
    {
      // We support 2 modes... one for single day entry, one for multiday entry.
      if ($this->m_insertmode=="multi")
      {
        $res = $this->m_attribList["activitydate"]->edit($record, $prefix, $mode);
        $toDate = new atkDateAttribute("todate", "F d Y","d F Y", 0, date("Ymd"), AF_OBLIGATORY);                
        
        $res.= " ".text("uptoandincluding")." ".$toDate->edit($record, $prefix, $mode);
        $res.= '<input type="hidden" name="'.$prefix.'activitydate[multi]" value="1">';
        $newmode = "day";
      }
      else
      {
        $res = $this->m_attribList["activitydate"]->edit($record, $prefix, $mode);      
        $newmode = "multi";
      }
      $res.= " ".href("dispatch.php?atknodetype=".$this->atkNodeType()."&atkaction=".$this->m_action."&insertmode=".$newmode, 
                      text("insert_".$newmode, $this->m_type, $this->m_module), 
                      SESSION_DEFAULT, 
                      true);
      return $res;
    }
    else
    {
      return $this->m_attribList["activitydate"]->edit($record, $prefix, $mode);
    }
  }
  
  function dayfilter_edit($record, $prefix="", $mode="")
  {
    $res= "<BR>".text("filterdays", $this->m_type, $this->m_module)."<BR>";
    $days = array("mon", "tue", "wed", "thu", "fri", "sat", "sun");    
    
    // we use a hidden variable to indicate that the dayfilter has been edited.
    // If it has not been edited, dayfilter[edited] will not be 1, and we will
    // assume a default dayfilter of mon-fri. TODOFIXME: take the correct
    // day values from the user's contract.
    $res.='<input type="hidden" name="'.$prefix.'dayfilter[edited]" value="1">';
    for ($i=0;$i<7;$i++)
    {
      if ($record["dayfilter"]["edited"]==1)
      {
        $checked = ($record["dayfilter"][$i]==1?"checked":"");
      }
      else
      {
        $checked = ($i<5?"checked":"");
      }
      $res.= text($days[$i]).': <input type="checkbox" name="'.$prefix.'dayfilter['.$i.']" value="1" '.$checked.'> &nbsp; ';
    }
    return $res;
  }
  
  function addDb(&$record, $exectrigger=true)
  {
    if ($record["activitydate"]["multi"]==1)
    {      
      // We're registering multiple days at once. 
      $walkerStamp = adodb_mktime(12,0,0,$record["activitydate"]["month"],$record["activitydate"]["day"],$record["activitydate"]["year"]);
      $walker = date("Y-m-d", $walkerStamp);      
      
      $todate = $this->m_postvars["todate"]; // todate is not part of $record, because the todate attrib isn't really a part of the node.
      
      $end = date("Y-m-d", adodb_mktime(12,0,0,$todate["month"],$todate["day"],$todate["year"]));
      
      // Make a quick lookup array in which we can easily see if a day was checked.
      $doDays=array();
      for ($i=0; $i<7; $i++)
      {
        if ($this->m_postvars["dayfilter"][$i]==1)
        {
          $doDays[] = (($i+1)%7); // we started with monday as 0, but php date() function counts sunday as 0.
        }
      }      
      
      for ($i=1; $walker <= $end ; $i++)
      {                       
        $dow = date("w", $walkerStamp);
        if (!in_array($dow, $doDays) || $this->getLockType($this->m_user,$walker)!="")
        {
          atkdebug("Skipping $walker, for this day is locked or not checked");
        }
        else
        {
          $newRecord = $record; // We're going to save the data we entered.
          // but replace the activitydate field with the walker:
          $newRecord["activitydate"] = array("year"=>date("Y", $walkerStamp),
                                             "month"=>date("m", $walkerStamp),
                                             "day"=>date("d", $walkerStamp));
          parent::addDb($newRecord);
        }
        
        // End of day, add 1 day and reloop.
        $walkerStamp = adodb_mktime(12,0,0,$record["activitydate"]["month"],
                                     $record["activitydate"]["day"]+$i,
                                     $record["activitydate"]["year"]);
        $walker = date("Y-m-d", $walkerStamp);
                              
      }
    }
    else
    {
      parent::addDb($record, $exectrigger);
    }
  }
 
  /**
   * This method is called by the framework to determine the allowed actions on 
   * the hours node.
   * In this case, the edit and delete actions are disabled if the record does
   * not belong to the current user.
   * TODO/FIXME: add some supervisor right so some users may edit other users' 
   * entries.
   */
  function recordActions($rec, &$actions, &$mraactions)
  {
    $user = getUser();        
    if ($user["id"]=="" || $rec["userid"]["id"]!=$user["id"] 
        || $this->getLockType($rec["userid"]["id"],
                              sprintf("%02d-%02d-%02d", $rec["activitydate"]["year"], 
                                                        $rec["activitydate"]["month"],
                                                        $rec["activitydate"]["day"]))!="")
    {
      if (!$this->allowed("any_user"))
      {
        unset($actions["edit"]);
        unset($actions["delete"]);
        $idx = array_search("delete", $mraactions);
        if ($idx!==false)
        {
          unset($mraactions[$idx]);
        }
      }
      else 
      {
        $actionbase = $_SERVER["PHP_SELF"].'?atknodetype='.$this->atknodetype().'&atkselector=[pk]&viewuser='.$this->m_user;
        $actions = array();
        $actions["edit"] = $actionbase."&atkaction=edit";
        $actions["delete"] = $actionbase."&atkaction=delete";
      }  
    }
  }
  
  function getLockIcon()
  {
    if ($this->m_lock!="")
    {
      // Week is locked
      return ' <img src="'.atkconfig("atkroot").'atk/images/lock.gif" border="0">';
    }
  }  

  function preAdd(&$record)
  {    
    $this->modifyfortime($record);
  }
  
  function preUpdate(&$record)
  {
    $this->modifyfortime($record);
  }
  
  function modifyfortime(&$record)
  {
    // if we don't have duration, then we have a begin and an end time
    // however, we still store it as a duration, only with a start time
    // so we need to remove the end time and add a 'time'
    if ($record['starttime'])
    {
      $this->performTimeChecks($record);
      convertHoursToDuration($record);
    }
  }
  
  function edit_values($defaults)
  {
    // Here we make sure that if we have a starttime (don't worry,
    // the convertDurationToHours() function checks for it) that
    // we convert the duration to a starttime and entime notation
    // for editting
    convertDurationToHours($defaults);
    return array('endtime'=>$defaults['endtime']);
  }

  /**
   * Does checks on a record with a starttime and endtime in it
   * before it is converted and goes into the database
   */
  function performTimeChecks(&$record)
  {
    // Check if the endtime isn't before the starttime
     if ($record["starttime"]["hours"]>$record["endtime"]["hours"] || 
        ($record["starttime"]["hours"]==$record["endtime"]["hours"] && $record["starttime"]["minutes"]>$record["endtime"]["minutes"]) || 
        ($record["starttime"]["hours"]==$record["endtime"]["hours"] && $record["starttime"]["minutes"]==$record["endtime"]["minutes"] && $record["starttime"]["seconds"]>$record["endtime"]["seconds"])) 
        triggerError($record, "starttime", "error_begintime_before_endtime");
        
    // Check if that period is still available
    
    // WARNING!
    // Here we do a selectDb on ourselves for the current activitydate, 
    // normally this would fail, however, because this function is only called in
    // a special mode and never directly from the constructor, it should always work.
    $select = $this->selectDb("activitydate = '".$record['activitydate']['year']."-".$record['activitydate']["month"]."-".$record['activitydate']["day"]."'");
    foreach ($select as $dbrecord)
    {
      convertDurationToHours($dbrecord);
      if (!($dbrecord["starttime"]["hours"]<$record["endtime"]["hours"] || 
           ($dbrecord["starttime"]["hours"]==$record["endtime"]["hours"] && $dbrecord["starttime"]["minutes"]<$record["endtime"]["minutes"]) || 
           ($dbrecord["starttime"]["hours"]==$record["endtime"]["hours"] && $dbrecord["starttime"]["minutes"]==$record["endtime"]["minutes"] && $dbrecord["starttime"]["seconds"]<$record["endtime"]["seconds"])) ||
          !($record["starttime"]["hours"]>$dbrecord["endtime"]["hours"] || 
            ($record["starttime"]["hours"]==$dbrecord["endtime"]["hours"] && $record["starttime"]["minutes"]>$dbrecord["endtime"]["minutes"]) || 
          ($record["starttime"]["hours"]==$dbrecord["endtime"]["hours"] && $record["starttime"]["minutes"]==$dbrecord["endtime"]["minutes"] && $record["starttime"]["seconds"]>$dbrecord["endtime"]["seconds"]))) 
      {
          triggerError($record, "starttime", "error_time_already_registered");
          return false;
      }
    }
    // END WARNING
  }
}
?>