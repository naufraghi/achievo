<?php

  /**
   * This file is part of the Achievo ATK distribution.
   * Detailed copyright and licensing information can be found
   * in the doc/COPYRIGHT and doc/LICENSE files which should be
   * included in the distribution.
   *
   * @package atk
   * @subpackage attributes
   *
   * @copyright (c)2000-2004 Ibuildings.nl BV
   * @license http://www.achievo.org/atk/licensing ATK Open Source License
   *
   * @version $Revision: 5.20 $
   * $Id: class.atkdatetimeattribute.inc,v 5.20 2006/04/14 15:16:18 peter Exp $
   */

  /**
   * @internal Includes
   */
  atkimport("atk.attributes.atkdateattribute");
  atkimport("atk.attributes.atktimeattribute");

  /**
   * The atkDateTimeAttribute class can be used for date and time entry.
   * It corresponds to a DATETIME field in the database.
   *
   * @author Sandy Pleyte <sandy@achievo.org>
   * @package atk
   * @subpackage attributes
   *
   */
  class atkDateTimeAttribute extends atkAttribute
  {
    var $m_time = "";
    var $m_date = "";
    

    /**
     * Converts a date array to a timestamp
     * year, month, day are obligatory !!
     *
     * @param array $dateArray Date Array
     * @return int Timestamp
     */
    function arrayToDateTime($dateArray)
    {
      $hour = 0;
      $min = 0;
      $sec = 0;
      $dateValid = true;

      if(!empty($dateArray["hours"])) $hour = $dateArray["hours"];
      if(!empty($dateArray["minutes"])) $min = $dateArray["minutes"];
      if(!empty($dateArray["seconds"])) $sec = $dateArray["seconds"];
      if(!empty($dateArray["day"])) $day = $dateArray["day"];
      else $dateValid = false;
      if(!empty($dateArray["month"])) $month = $dateArray["month"];
      else $dateValid = false;
      if(!empty($dateArray["year"])) $year = $dateArray["year"];
      else $dateValid = false;

      if($dateValid) return adodb_mktime($hour,$min,$sec,$month,$day,$year);
      else return adodb_mktime(0,0,0);
    }    

    /**
     * Constructor
     *
     * @todo I don't know who added the default_date and default_time parameters,
     *       but at least the atkDateAttribute doesn't support a default date. if
     *       you want to set a default date you should use initial_values anyways
     *       so I think the default_date / default_time parameters should be removed.
     *       But can we keep this backwards compatible somehow? At least now we
     *       are certain it doesn't work. ;) (PCV)
     *
     * @param $name        Name of the attribute
     * @param $default_date   start date
     * @param $default_time     start time
     * @param $flags Flags for this attribute
     */
    function atkDateTimeAttribute($name, $default_date="",$default_time="",$flags=0)
    {
      $default_steps = array();
      for($i=0;$i<60;$i++)
      {
        $default_steps[$i] = $i;
      }

      if (is_numeric($default_date))
      {
        $flags = $default_date;
        $default_date = "";
      }


      if($default_date=="") { $default_date = ""; }
      if($default_time=="") { $default_time = date("H:i"); }

      $this->m_time = new atkTimeattribute($name,0,23,$default_steps,$default_time, $flags);
      $this->m_date = new atkDateattribute($name,'','',0,0,$flags);

      $this->atkAttribute($name,$flags); // base class constructor
    }

    function validate(&$record, $mode)
    {
      $this->m_date->validate($record,$mode);
      $this->m_time->validate($record,$mode);
    }

    /**
     * Converts a date/time string (YYYYMMDDHHMISS) to an
     * array with 5 fields (day, month, year, hours, minutes, seconds).
     * Defaults to current date/time.
     *
     * @param $datetime the time string
     * @return array with 6 fields (day, month, year, hours, minutes, seconds)
     */
    function datetimeArray($datetime=NULL)
    {
      if ($datetime == NULL)
        $datetime = date("YmdHi");
      $date = substr($datetime, 0, 8);
      $time = substr($datetime, 8, 6);
      return array_merge(atkDateAttribute::dateArray($date), atkTimeAttribute::timeArray($time));
    }

    function init()
    {

      $this->m_time->m_owner=$this->m_owner;
      $this->m_date->m_owner=$this->m_owner;
      $this->m_time->m_ownerInstance=&$this->m_ownerInstance;
      $this->m_date->m_ownerInstance=&$this->m_ownerInstance;
    }


    function fetchMeta($metadata)
    {
      $this->m_date->fetchMeta($metadata);
      $this->m_time->fetchMeta($metadata);
    }

    /**
     * Display's html version of Record
     * @param $record
     * @return text string of $record
     */
    function display($record, $mode="")
    {
      $date = $this->m_date->display($record, $mode);
      $time = $this->m_time->display($record, $mode);
      if($date!='' && $time!='')
        return $date.(($mode=="csv"||$mode=="plain")?" ":"&nbsp;").$time;
      else
        return "";
    }

    function fetchValue($postvars)
    {
      $date = $this->m_date->fetchValue($postvars);
      $time = $this->m_time->fetchValue($postvars);
      if ($date == NULL) return NULL;
      if ($time == NULL) $time = array('hours' => 0, 'minutes' => 0, 'seconds' => 0);
      else return array_merge($date, $time);
    }

    /**
     * Returns a piece of html code that can be used in a form to edit this
     * attribute's value.
     * @param $record Array with fields
     * @return piece of html code with a checkbox
     */
    function edit($record="", $fieldprefix="")
    {
      return $this->m_date->edit($record,$fieldprefix)."&nbsp;&nbsp;-&nbsp;&nbsp;".$this->m_time->edit($record,$fieldprefix);
    }


    function value2db($rec)
    {
      //$datetime = split(" ",$rec[$this->fieldname()]);
      //$tmp_rec = $rec;
      //$tmp_rec[$this->fieldname()]=$datetime[0];
      $date=$this->m_date->value2db($rec);

      //$tmp_rec = $rec;
      //$tmp_rec[$this->fieldname()]=$datetime[1];
      $time=$this->m_time->value2db($rec);

      if($time!='NULL' && $date!='NULL')
        $result = $date." ".$time;
      else
        $result = 'NULL';
     return $result;
    }
    /**
     * Convert database value to datetime array
     * @param $rec database record with date field
     * @return array with 3 fields (hours:minutes:seconds)
     */
    function db2value($rec)
    {
      if(isset($rec[$this->fieldName()]) && $rec[$this->fieldName()] != NULL)
      {
        /**
         * @todo Fix handling of 0 and NULL db values in the date, time and datetime attributes
         * Currently the date attribute gives an empty string when parsing 0000-00-00,
         * the time attribute gives an array with all three values set to 00,
         * and the datetimeattribute gives an empty string now (previously it gave a php warning
         * because it was trying to array_merge the empty string from the date attribute with the
         * array of the time attribute).
         */
        if ($rec[$this->fieldName()] == "0000-00-00 00:00:00")
          return "";

        $datetime = split(" ",$rec[$this->fieldname()]);

        $tmp_rec = $rec;
        $tmp_rec[$this->fieldname()]=$datetime[0];
        $result_date=$this->m_date->db2value($tmp_rec);
        if ($result_date == NULL) return NULL;

        $tmp_rec = $rec;
        $tmp_rec[$this->fieldname()]=$datetime[1];
        $result_time = $this->m_time->db2value($tmp_rec);
        if ($result_time == NULL)
          $result_time = array('hours' => 0, 'minutes' => 0, 'seconds' => 0);

        return array_merge($result_date,$result_time);
      }
      else return NULL;
    }

    /**
     * Add's fields to query
     * @param $query The SQL query
     * @param $tablename Tablename
     * @param $fieldaliasprefix Field alias prefix
     * @param $level ?
     * @param $mode select/admin/copy/edit
     * @return New SQL Query
     */
    function addToQuery(&$query, $tablename="", $fieldaliasprefix="", $rec="", $level, $mode)
    {
      if ($mode == "add" || $mode == "update")
      {
        if($this->value2db($rec) == '' || $this->value2db($rec) == 'NULL')
        {
          $query->addField($this->fieldName(),'NULL','','',false);
        }
        else
        {
          $db = &$this->m_ownerInstance->getDb();
          if ($db->getType() != 'oci9')
            $query->addField($this->fieldName(),$this->value2db($rec),"","",!$this->hasFlag(AF_NO_QUOTES));
          else
          {
            $value = $this->value2db($rec);
            $query->addField($this->fieldName(),$value,"","",!$this->hasFlag(AF_NO_QUOTES), $mode, "DATETIME");
          }
        }
      }
      else
      {
        if (atkconfig('database') != 'oci9')
          $query->addField($this->fieldName(),"",$tablename,$fieldaliasprefix,!$this->hasFlag(AF_NO_QUOTES));
        else
        {
          $query->addField($this->fieldName(),"",$tablename,$fieldaliasprefix,!$this->hasFlag(AF_NO_QUOTES), $mode, "DATETIME");
        }
      }
    }

    /**
     * Returns a piece of html code that can be used in a form to search values
     * @param $record Array with fields
     * @return piece of html code with a checkbox
     */
    function search($record="", $extended=false, $fieldprefix="")
    {
     return $this->m_date->search($record, $extended, $fieldprefix);
    }
    
    function searchCondition(&$query, $table, $value, $searchmode)
    {
      $this->m_date->searchCondition($query, $table, $value, $searchmode);
    }    
    

    function hide($record="", $fieldprefix="")
    {
      // we only need to return the date part, because the dateattribute also
      // hides the other (time) elements that are present in the record (is that
      // a bug of the dateattribute?)
      return $this->m_date->hide($record, $fieldprefix);
    }

    function getSearchModes()
    {
      return $this->m_date->getSearchModes();
    }

    function dbFieldType()
    {
      // TODO FIXME: Is this correct? Or does the datetimeattribute currently only support varchar fields?
      return "datetime";
    }

    function parseStringValue($stringvalue)
    {
	    $datetime = split(" ",$stringvalue);
		  $formatsdate = array("dd-mm-yyyy","dd-mm-yy","d-mm-yyyy","dd-m-yyyy","d-m-yyyy","yyyy-mm-dd","yyyy-mm-d","yyyy-m-dd","yyyy-m-d");
		  $retval=array_merge(atkDateAttribute::parseDate($datetime[0],$formatsdate),
		                      atkTimeAttribute::parseTime($datetime[1]));
		  return $retval;
    }

  }
?>