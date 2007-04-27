<?php

  /**
   * This file is part of the Achievo ATK distribution.
   * Detailed copyright and licensing information can be found
   * in the doc/COPYRIGHT and doc/LICENSE files which should be
   * included in the distribution.
   *
   * @package atk
   * @subpackage recordlist
   *
   * @copyright (c)2000-2004 Ibuildings.nl BV
   * @license http://www.achievo.org/atk/licensing ATK Open Source License
   *
   * @version $Revision: 5.35 $
   * $Id: class.atkrecordlist.inc,v 5.35 2006/05/25 12:13:33 ivo Exp $
   */

  /** recordlist flags */
  define("RL_NO_SORT",                         1); // recordlist is not sortable
  define("RL_NO_SEARCH",                       2); // recordlist is not searchable
  define("RL_NO_EXTENDED_SEARCH",              4); // recordlist is not searchable
  define("RL_EMBED",                           8); // recordlist is embedded
  define("RL_MRA",                            16); // multi-record-actions enabled
  define("RL_MRPA",                           32); // multi-record-priority-actions enabled
  define("RL_LOCK",                           64); // records can be locked
  define("RL_EXT_SORT",                      128); // extended sort feature

  atkimport("atk.utils.atkstringparser");

  /**
   * The recordlist class is used to render tables containing records.
   *
   * @author Ivo Jansch <ivo@achievo.org>
   * @package atk
   * @subpackage recordlist
   *
   */
  class atkRecordList
  {
    var $m_node=NULL;
    var $m_flags=0;
    var $m_actionloader;

    /**
     * @access private
     */
    function setNode(&$node)
    {
      $this->m_node = &$node;
    }

    /**
     * Converts the given node flags to recordlist flags where possible.
     *
     * @param Integer $flags
     * @static
     */
    function convertFlags($flags)
    {
      $result  = hasFlag($flags, NF_MRA) ? RL_MRA : 0;
      $result |= hasFlag($flags, NF_MRPA) ? RL_MRPA : 0;
      $result |= hasFlag($flags, NF_LOCK) ? RL_LOCK : 0;
      $result |= hasFlag($flags, NF_NO_SEARCH) ? RL_NO_SEARCH : 0;
      $result |= hasFlag($flags, NF_NO_EXTENDED_SEARCH) ? RL_NO_EXTENDED_SEARCH : 0;
      $result |= hasFlag($flags, NF_EXT_SORT) ? RL_EXT_SORT : 0;
      return $result;
    }

    /**
     * Creates a normal recordlist based on the given parameters.
     * @param Array   $recordset    the list of records
     * @param Array   $actions      the default actions array
     * @param Integer $flags        recordlist flags (see the top of this file)
     * @param Array   $suppressList fields we don't display
     * @param String  $formName     if embedded the form name in which we are embedded
     * @param Array   $navigation   Navigation links
     * @param String  $embedprefix  The prefix for embeded fields
     * @return String The rendered recordlist
     */
    function render(&$node, $recordset, $actions, $flags=0, $suppressList="", $formName="", $navigation=array(),$embedprefix="")
    {
      $this->setNode($node);
      $this->m_flags = $flags;

      $theme = &atkinstance("atk.ui.atktheme");
      $page = &atkPage::getInstance();
      $page->register_style($theme->stylePath("recordlist.css"));

      $listName = "rl_".getUniqueId("normalRecordList");
      $page->register_script(atkconfig("atkroot")."atk/javascript/recordlist.js");

  	  $defaulthighlight = $theme->getAttribute("highlight");
      $selectcolor = $theme->getAttribute("select");

      /* retrieve list array */
      $list = $this->listArray($recordset, $flags, "", $actions, $suppressList, $embedprefix);

      /* Check if some flags are still valid or not... */
      if (hasFlag($flags, RL_MRA) && (count($list["mra"]) == 0 || count($list["rows"]) == 0)) $flags ^= RL_MRA;
      if (!hasFlag($flags, RL_NO_SEARCH) && count($list["search"]) == 0) $flags |= RL_NO_SEARCH;
      if (hasFlag($flags, RL_MRPA) && (count($this->m_node->m_priority_actions) == 0 || count($list["rows"]) == 0)) $flags ^= RL_MRPA;
      elseif (hasFlag($flags, RL_MRPA))
      {
        $flags = ($flags | RL_MRA | RL_MRPA ) ^ RL_MRA;
        if ($this->m_node->m_priority_max == 0)
          $this->m_node->m_priority_max = $this->m_node->m_priority_min + count($list["rows"]) - 1;
      }

      $orientation  = atkconfig("recordlist_orientation");
      if (trim(atkconfig("recordlist_vorientation")) != "")
        $vorientation = atkconfig("recordlist_vorientation");
      else $vorientation = 'top';

      $ui = &$this->m_node->getUi();

      if (is_object($ui) && is_object($page))
      {

        /**************/
        /* HEADER ROW */
        /**************/
        $headercols = array();

        if (hasFlag($flags, RL_MRA) || hasFlag($flags, RL_MRPA))
        {
          $headercols[] = array("content"=>""); // Empty leader on top of mra action list.
        }
        if (hasFlag($flags, RL_LOCK))
        {
          $headercols[] = array("content"=>'<img src="'.atkconfig("atkroot").'atk/images/lock_head.gif">');
        }
        if ($orientation == "left" || $orientation == "both")
        {
          $headercols[] = array("content"=>"");
        }

        foreach (array_values($list["heading"]) as $head)
        {
          if (hasFlag($this->m_flags, RL_EMBED) && !empty($head["url"]))
          {
            $head["url"] = str_replace("atkorderby=","atkorderby{$embedprefix}=",$head["url"]);
          }

          if (empty($head["url"]))
          {
            $headercols[] = array("content"=>$head["title"]);
          }
          else
          {
            $headercols[] = array("content"=>href($head["url"], $head["title"]));
          }
        }

        if ($orientation == "right" || $orientation == "both")
        {
          $headercols[] = array("content"=>"");
        }

        /**************/
        /* SORT   ROW */
        /**************/
        $sortcols = array();
        $sortstart=""; $sortend="";
        if (hasFlag($flags, RL_EXT_SORT))
        {
          $button = '<input type="submit" value="'.text("sort").'">';
          if (hasFlag($flags, RL_MRA) || hasFlag($flags, RL_MRPA))
          {
            $sortcols[] = array("content"=>""); // Empty leader on top of mra action list.
          }
          if (hasFlag($flags, RL_LOCK))
          {
            $sortcols[] = array("content"=>"");
          }
          if ($orientation == "left" || $orientation == "both")
          {
            $sortcols[] = array("content"=>$button);
          }

          $sortstart = '<a name="sortform"></a>'.
                         '<form action="'.$_SERVER["PHP_SELF"].'?'.SID.'" method="get">'.
                         session_form().
                           '<input type="hidden" name="atkstartat" value="0">'; // reset atkstartat to first page after a new sort

          foreach (array_keys($list["heading"]) as $key)
          {
            if (isset($list["sort"][$key])) $sortcols[] = array("content"=>$list["sort"][$key]);
          }

          $sortend = '</form>';

          if ($orientation == "right" || $orientation == "both")
          {
            $headercols[] = array("content"=>$button);
          }
        }

        /**************/
        /* SEARCH ROW */
        /**************/

        $searchcols = array();
        $searchstart=""; $searchend="";
        if (!hasFlag($flags, RL_NO_SEARCH))
        {
          if (isset($this->m_node->m_postvars["atksearchmode"]))
          {
            $searchmode = $this->m_node->m_postvars["atksearchmode"];
          }
          if (empty($searchmode)) $searchmode = atkconfig("search_defaultmode");

          /* Get the first one, because they are all the same for now
           * TODO: every attrib needs to get his own searchmode
           */
          if (is_array($searchmode))
          {
            $keys = array_keys($searchmode);
            $searchmode = $searchmode[$keys[0]];
          }

          $button = '<input type="submit" class="btn_search" value="'.atktext("search").'">';
          if (!hasFlag($flags, RL_NO_EXTENDED_SEARCH)&&!$this->m_node->hasFlag(NF_NO_EXTENDED_SEARCH))
          {
            $button .= '<br>'.href($_SERVER["PHP_SELF"]."?atknodetype=".$this->m_node->atkNodeType()."&atkaction=".$node->getExtendedSearchAction()."&atksearchmode=".
                                               $searchmode,"(".atktext("search_extended").")",SESSION_NESTED);
          }

          $searchstart = '<a name="searchform"></a>';
          if (!hasFlag($this->m_flags, RL_EMBED))
          {
            $searchstart.='<form action="'.$_SERVER["PHP_SELF"].'?'.SID.'" method="get">'.session_form();
            $searchstart.= '<input type="hidden" name="atknodetype" value="'.$this->m_node->atkNodeType().'">'.
                           '<input type="hidden" name="atkaction" value="'.$this->m_node->m_action.'">'.
                           '<input type="hidden" name="atksearchmode" value="'.$searchmode.'">'.
                           '<input type="hidden" name="atksmartsearch" value="clear">'.
                           '<input type="hidden" name="atkstartat" value="0">'; // reset atkstartat to first page after a new search;
          }

          if (hasFlag($flags, RL_MRA) || hasFlag($flags, RL_MRPA))
          {
            $searchcols[] = array("content"=>"");
          }
          if (hasFlag($flags, RL_LOCK))
          {
            $searchcols[] = array("content"=>"");
          }
          if ($orientation == "left" || $orientation == "both")
          {
            $searchcols[] = array("content"=>$button);
          }

          foreach (array_keys($list["heading"]) as $key)
          {
            if (isset($list["search"][$key]))
            {
              $searchcols[] = array("content"=>$list["search"][$key]);
            }
            else
            {
              $searchcols[] = array("content"=>"");
            }
          }
          if ($orientation == "right" || $orientation == "both")
          {
            $searchcols[] = array("content"=>$button);
          }

          $searchend = "";
          if (!hasFlag($this->m_flags, RL_EMBED)) $searchend = '</form>';
        }

        /*******************************************/
        /* MULTI-RECORD-(PRIORITY-)ACTIONS FORM DATA */
        /*******************************************/
        $liststart = "";
        $listend = "";
        if (hasFlag($flags, RL_MRA) || hasFlag($flags, RL_MRPA))
        {
          $page->register_script(atkconfig("atkroot")."atk/javascript/formselect.js");

          if (!hasFlag($flags, RL_EMBED))
          {
            if (empty($formName)) $formName = $listName;
            $liststart = '<form name="'.$formName.'" method="post">'.
                       session_form(SESSION_DEFAULT).
                       '<input type="hidden" name="atknodetype" value="'.$this->m_node->atkNodeType().'">'.
                       '<input type="hidden" name="atkaction" value="'.$this->m_node->m_action.'">';
          }

          if (hasFlag($flags, RL_MRA))
          {
            $liststart.= '<script language="javascript" type="text/javascript">var '.$listName.' = new Object();</script>';
          }
        }

        /********/
        /* ROWS */
        /********/

        $records = array();
        $keys = array_keys($actions);
        $actionloader = "rl_a['".$listName."'] = new Array();";
        $actionloader.= "\nrl_a['".$listName."']['base'] = '".session_vars(SESSION_NESTED,1,$actions[$keys['0']])."';";
        $actionloader.= "\nrl_a['".$listName."']['embed'] = ".(hasFlag($flags, RL_EMBED)?'true':'false').";";

        if (isset($navigation["next"]) && isset($navigation["next"]["url"]))
        {
          $actionloader.="\nrl_a['".$listName."']['next'] = '".$navigation["next"]["url"]."';";
        }
        if (isset($navigation["previous"]) && isset($navigation["previous"]["url"]))
        {
          $actionloader.="\nrl_a['".$listName."']['previous'] = '".$navigation["previous"]["url"]."';";
        }

        for ($i = 0, $_i = count($list["rows"]); $i < $_i; $i++)
        {
          $record = array();

          /* Special rowColor method makes it possible to change the row color based on the record data.
           * the method can return a simple value (which will be used for the normal row color), or can be
           * an array, in which case the first element will be the normal row color, and the second the mouseover
           * row color, example: function rowColor(&$record, $num) { return array('red', 'blue'); }
           */
          $method = "rowColor";
          $bgn="";
          $bgh = $defaulthighlight;
          if (method_exists($this->m_node, $method))
          {
            $bgn = $this->m_node->$method($recordset[$i], $i);
            if (is_array($bgn)) list($bgn, $bgh) = $bgn;
          }


          /* alternate colors of rows */
          $record["background"] = $bgn;
          $record["highlight"] = $bgh;
          $record["rownum"] = $i;
          $record["id"] = $listName.'_'.$i;
          $record["type"] = $list["rows"][$i]["type"];

          /* multi-record-priority-actions -> priority selection */
          if (hasFlag($flags, RL_MRPA))
          {
            $select = '<select name="'.$listName.'_atkselector[]">'.
                        '<option value="'.rawurlencode($list["rows"][$i]["selector"]).'"></option>';
            for ($j = $this->m_node->m_priority_min; $j <= $this->m_node->m_priority_max; $j++)
              $select .= '<option value="'.$j.'">'.$j.'</option>';
            $select .= '</select>';
            $record["cols"][] = array("content" => $select);
          }

          /* multi-record-actions -> checkbox */
          elseif (hasFlag($flags, RL_MRA))
          {
            if (count($list["rows"][$i]["mra"]) > 0)
              $record["cols"][] = array(
                           "content"=>'<input type="checkbox" name="'.$listName.'_atkselector[]" value="'.rawurlencode($list["rows"][$i]["selector"]).'" onclick="if (this.disabled) this.checked = false">'.
                                      '<script language="javascript"  type="text/javascript">'.$listName.'["'.rawurlencode($list["rows"][$i]["selector"]).'"] = new Array("'.implode($list["rows"][$i]["mra"], '","').'");</script>');

            else $record["cols"][] = array("content"=>"");
          }

          /* locked? */
          if (hasFlag($flags, RL_LOCK))
          {
            if (is_array($list["rows"][$i]["lock"]))
            {
              $alt = $list["rows"][$i]["lock"]["user_id"]." / ".$list["rows"][$i]["lock"]["user_ip"];
              $record["cols"][] = array("content"=>'<img src="'.atkconfig("atkroot").'atk/images/lock.gif" alt="'.$alt.'" border="0">');
            }
            else $record["cols"][] = array("content"=>"");
          }

          $str_actions = "<span class=\"actions\">";
          $actionloader.="\nrl_a['".$listName."'][".$i."] = new Array();";
					$icons = atkconfig("recordlist_icons", true);

          foreach ($list["rows"][$i]["actions"] as $name => $url)
          {
      	    $actionloader.="\nrl_a['".$listName."'][".$i."]['".$name."'] = '".$url."';";

    	      $link = $name;

    	      if($icons == true)
    	      {
    	        $icon = $theme->iconPath(strtolower($name),"recordlist", $this->m_node->m_module);
      	      $link = '<img width="16" height="16" border="0" src="'.$icon.'" title="'.atktext($name, $this->m_node->m_module, $this->m_node->m_type).'">';
    	      }
    	      else
    	      {
    	        $link = atktext($name);
	          }

            $confirmtext= "false";
            if (atkconfig("recordlist_javascript_delete") && $name=="delete")
            $confirmtext = "'".$this->m_node->confirmActionText($name)."'";
            $str_actions.='<a href="'."javascript:rl_do('$listName',$i,'$name',$confirmtext);".'">'.$link.'</a>&nbsp;';
          }

          $str_actions.="</span>";
          /* actions (left) */
          if ($orientation == "left" || $orientation == "both")
          {
            if (!empty($str_actions))
            {
              $record["cols"][] = array("content"=>$str_actions);
            }
            else  $record["cols"][] = array("content"=>"");

          }

          /* columns */
          foreach ($list["rows"][$i]["data"] as $html)
            $record["cols"][] = array("content"=>$html);

          /* actions (right) */
          if ($orientation=="right"||$orientation=="both")
          {
            if (!empty($str_actions)) $record["cols"][] = array("content"=>$str_actions);
            else $record["cols"][] = array("content"=>"");
          }

          $records[] = $record;

        }

        $page->register_loadscript($actionloader);
        $this->m_actionloader = $actionloader;

        /*************/
        /* TOTAL ROW */
        /*************/
        $totalcols = array();

        if (count($list["total"]) > 0)
        {
          if (hasFlag($flags, RL_MRA) || hasFlag($flags, RL_MRPA)) $totalcols[] = array("content"=>"");
          if (hasFlag($flags, RL_LOCK)) $totalcols[] = array("content"=>"");
          if ($orientation == "left" || $orientation == "both") $totalcols[] = array("content"=>"");

          foreach (array_keys($list["heading"]) as $key)
          {
            $totalcols[] = array("content"=>(isset($list["total"][$key])?$list["total"][$key]:""));
          }

          if ($orientation == "right" || $orientation == "both") $totalcols[] = array("content"=>"");
        }

        /*************************************************/
        /* MULTI-RECORD-PRIORITY-ACTION FORM (CONTINUED) */
        /*************************************************/
        $mra = "";
        if (hasFlag($flags, RL_MRPA))
        {
          $target = session_url($_SERVER["PHP_SELF"].'?atknodetype='.$this->m_node->atkNodeType(), SESSION_NESTED);

          /* multiple actions -> dropdown */
          if (count($this->m_node->m_priority_actions) > 1)
          {
            $mra = '<select name="'.$listName.'_atkaction">'.
                         '<option value="">'.atktext("with_selected").':</option>';

            foreach ($this->m_node->m_priority_actions as $name)
              $mra .= '<option value="'.$name.'">'.atktext($name).'</option>';

            $mra .= '</select>&nbsp;'.$this->getCustomMraHtml().
                       '<input type="button" class="btn" value="'.atktext("submit").'" onclick="atkSubmitMRPA(\''.$listName.'\', this.form, \''.$target.'\')">';
          }

          /* one action -> only the submit button */
          else
          {
            $mra= $this->getCustomMraHtml().'<input type="hidden" name="'.$listName.'_atkaction" value="'.$this->m_node->m_priority_actions[0].'">'.
                       '<input type="button" class="btn" value="'.atktext($this->m_node->m_priority_actions[0]).'" onclick="atkSubmitMRPA(\''.$listName.'\', this.form, \''.$target.'\')">';
          }

          if (!hasFlag($flags, RL_EMBED))
          {
            $listend= '</form>';
          }
        }

        /****************************************/
        /* MULTI-RECORD-ACTION FORM (CONTINUED) */
        /****************************************/
        elseif (hasFlag($flags, RL_MRA))
        {
          $target = session_url($_SERVER["PHP_SELF"].'?atknodetype='.$this->m_node->atkNodeType(), SESSION_NESTED);

          $mra = (count($list["rows"]) > 1 ?
                      '<a href="javascript:updateSelection(\''.$listName.'\', document.forms[\''.$formName.'\'], \'all\')">'.atktext("select_all").'</a> / '.
                      '<a href="javascript:updateSelection(\''.$listName.'\', document.forms[\''.$formName.'\'], \'none\')">'.atktext("deselect_all").'</a> / '.
                      '<a href="javascript:updateSelection(\''.$listName.'\', document.forms[\''.$formName.'\'], \'invert\')">'.atktext("select_invert").'</a>'
                     :
                      '<img src="'.atkconfig("atkroot").'atk/images/dummy.gif" border="0" height="1" width="1">');

          /* multiple actions -> dropdown */
          if (count($list["mra"]) > 1)
          {
            $mra .= '<select name="'.$listName.'_atkaction" onchange="javascript:updateSelectable(\''.$listName.'\', this.form)">'.
                       '<option value="">'.atktext("with_selected").':</option>';

            foreach ($list["mra"] as $name)
              $mra .= '<option value="'.$name.'">'.atktext($name, $this->m_node->m_module, $this->m_node->m_type).'</option>';

            $mra .= '</select>&nbsp;'.$this->getCustomMraHtml().
                       '<input type="button" class="btn" value="'.atktext("submit").'" onclick="atkSubmitMRA(\''.$listName.'\', this.form, \''.$target.'\')">';
          }

          /* one action -> only the submit button */
          else
          {
            $mra .= '&nbsp; <input type="hidden" name="'.$listName.'_atkaction" value="'.$list["mra"][0].'">'.
            $this->getCustomMraHtml().
                       '<input type="button" class="btn" value="'.atktext($list["mra"][0],$this->m_node->m_module, $this->m_node->m_type).'" onclick="atkSubmitMRA(\''.$listName.'\', this.form, \''.$target.'\')">';
          }

          if (!hasFlag($flags, RL_EMBED)) $listend = '</form>';
        }

        if (atkconfig("use_keyboard_handler"))
        {
          $kb = &atkKeyboard::getInstance();
          $kb->addRecordListHandler($listName, $selectcolor, count($records));
        }

        $res = $ui->render("recordlist.tpl", array("vorientation"=>$vorientation,
                                                   "rows"=>$records,
                                                   "header"=>$headercols,
                                                   "search"=>$searchcols,
                                                   "sort"=>$sortcols,
                                                   "total"=>$totalcols,
                                                   "searchstart"=>$searchstart,
                                                   "searchend"=>$searchend,
                                                   "sortstart"=>$sortstart,
                                                   "sortend"=>$sortend,
                                                   "liststart"=>$liststart,
                                                   "listend"=>$listend,
                                                   "listid"=>$listName,
                                                   "mra"=>$mra));
        return $res;
      }
    }

    function getCustomMraHtml()
    {
      if (method_exists($this->m_node, "getcustommrahtml"))
      {
        $output = $this->m_node->getCustomMraHtml();
        return $output;
      }
    }

    /**
     * Function outputs an array with all information necessary to output a recordlist.
     *
     * @param Array   $recordset    List of records that need to be displayed
     * @param Integer $flags        Recordlist flags
     * @param String  $prefix       Prefix for each column name (used for subcalls)
     * @param Array   $actions      List of default actions for each record
     * @param Array   $suppress     An array of fields that you want to hide
     * @param String  $embedprefix  The prefix for embeded fields
     *
     * The result array contains the following information:
     *  "heading"  => for each visible column an array containing: "title" {, "url"}
     *  "search"   => for each visible column HTML input field(s) for searching
     *  "rows"     => list of rows, per row: "data", "actions", "mra", "record"
     *  "totalraw" => for each totalisable column the sum value field(s) (raw)
     *  "total"    => for each totalisable column the sum value (display)
     *  "mra"      => list of all multi-record actions
     *
     * @return see above
     */
    function listArray(&$recordset, $flags=0, $prefix="", $actions=array(), $suppress=array(), $embedprefix="")
    {
      if (!is_array($suppress)) $suppress = array();
      $result = array("heading" => array(), "search" => array(), "rows" => array(),
                      "totalraw" => array(), "total" => array(), "mra" => array());

      if (hasFlag($this->m_flags, RL_EMBED) && $embedprefix)
      {
        $prefix = $embedprefix."][";
      }

      $columnConfig = &$this->m_node->getColumnConfig();

      /* get the heading and search columns */
      $atksearchpostvar = isset($this->m_node->m_postvars["atksearch"]) ? $this->m_node->m_postvars["atksearch"] : null;
      if (!hasFlag($flags, RL_NO_SEARCH)) $this->m_node->setAttribSizes();
      foreach (array_keys($this->m_node->m_attribIndexList) as $r)
      {
        $name = $this->m_node->m_attribIndexList[$r]["name"];
        if (!in_array($name, $suppress))
        {
          $attribute = &$this->m_node->m_attribList[$name];
          $attribute->addToListArrayHeader($this->m_node->m_action, $result, $prefix, $flags, $atksearchpostvar, $columnConfig);
        }
      }

      /* actions array can contain multi-record-actions */
      if (count($actions) == 2 && count(array_diff(array_keys($actions), array("actions", "mra"))) == 0)
      {
        $mra = $actions["mra"];
        $actions = $actions["actions"];
      }
      else
        $mra = $this->m_node->hasFlag(NF_NO_DELETE) ? array() : array("delete");

      /* get the rows */
      for ($i = 0, $_i = count($recordset); $i < $_i; $i++)
      {
        $result["rows"][$i] = array("columns" => array(), "actions" => $actions, "mra" => $mra, "record" => &$recordset[$i], "data" => array());
        $result["rows"][$i]["selector"] = $this->m_node->primaryKey($recordset[$i]);
        $result["rows"][$i]["type"]="data";
        $row = &$result["rows"][$i];

        /* locked */
        if (hasFlag($flags, RL_LOCK))
        {
          $result["rows"][$i]["lock"] = $this->m_node->m_lock->isLocked($result["rows"][$i]["selector"], $this->m_node->m_table);
          if (is_array($result["rows"][$i]["lock"]))
          {
            unset($row["actions"]["edit"]);
            unset($row["actions"]["delete"]);
            $row["mra"] = array();
          }
        }

        /* actions / mra */
        $this->m_node->recordActions($row["record"], $row["actions"], $row["mra"]);
        $result["mra"] = array_merge($result["mra"], $row["mra"]);
        foreach($row["actions"] as $name => $url)
        {
          if (!empty($url) && $this->m_node->allowed($name, $row["record"]))
          {
            /* dirty hack */
            $atkencoded = strpos($url, "_15B") > 0;

            $url = str_replace("%5B","[",     $url);
            $url = str_replace("%5D","]",     $url);
            $url = str_replace("_1"."5B","[", $url);
            $url = str_replace("_1"."5D","]", $url);

            if ($atkencoded) $url = str_replace('[pk]', atkurlencode(rawurlencode($row["selector"]), false), $url);
            else $url = str_replace('[pk]', rawurlencode($row["selector"]), $url);

            $parser = new atkStringParser($url);
            $url = $parser->parse($row["record"],true);
            $row["actions"][$name] = $url;
          }
          else
          {
            unset($row["actions"][$name]);
          }
        }

        /* columns */
        foreach (array_keys($this->m_node->m_attribIndexList) as $r)
        {
          $name = $this->m_node->m_attribIndexList[$r]["name"];
          if (!in_array($name, $suppress))
          {
            $attribute = &$this->m_node->m_attribList[$name];
            $attribute->addToListArrayRow($this->m_node->m_action, $result, $i, $prefix, $flags);
          }
        }
      }

      if (hasFlag($flags, RL_EXT_SORT) && $columnConfig->hasSubTotals())
      {
        atkimport("atk.recordlist.atktotalizer");
        $totalizer = &new atkTotalizer($this->m_node, $columnConfig);
        $result["rows"] = $totalizer->totalize($result["rows"]);
      }

      if (hasFlag($flags, RL_MRA))
        $result["mra"] = array_values(array_unique($result["mra"]));

      return $result;
    }
  }

?>