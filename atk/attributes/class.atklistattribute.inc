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
   * @copyright (c)2000-2004 Ivo Jansch
   * @license http://www.achievo.org/atk/licensing ATK Open Source License
   *
   * @version $Revision: 5.33 $
   * $Id: class.atklistattribute.inc,v 5.33 2006/05/03 13:05:55 sandy Exp $
   */

  atkimport("atk.attributes.atkattribute");

  /**
   * Do not translate the options
   */
  define("AF_NO_TRANSLATION", AF_SPECIFIC_1);

   /**
    * Do not add a default null option.
    */
   define("AF_LIST_NO_OBLIGATORY_NULL_ITEM", AF_SPECIFIC_2);

  /**
   * Do not add null option ever
   */
  define("AF_LIST_NO_NULL_ITEM", AF_SPECIFIC_3);


  /**
   * The atkListAttribute class represents an attribute of a node
   * that has a selectbox to select from predefined values.
   *
   * @author Ivo Jansch <ivo@achievo.org>
   * @package atk
   * @subpackage attributes
   *
   */
  class atkListAttribute extends atkAttribute
  {
    /**
     * Array with options for Listbox
     */
    var $m_options = array();

    /**
     * Array with values for Listbox
     */
    var $m_values = array();

    /**
     * Array for fast lookup of what value belongs to what option.
     */
    var $m_lookup = array();

    /**
     * Attribute that is to be selected
     */
    var $m_selected;

    /**
     * Value that is used when list is empty, normally empty
     */
     var $m_emptyvalue;

     /**
      * The width of the dropdown list in pixels
      * @var int
      */
     var $m_width;

     var $m_onchangehandler_init = "newvalue = el.options[el.selectedIndex].value;\n";

    /**
     * Constructor.
     *
     * Warning: very old versions of this attribute supported passing the
     * parameters in a different order: $name, $flags, $optionArray.
     * This order used to be supported even when the new order was
     * implemented, but it has now been removed. Keep this in mind
     * when upgrading from a very old ATK version (pre ATK4).
     *
     * @param $name Name of the attribute
     * @param $optionArray Array with options
     * @param $valueArray Array with values. If you don't use this parameter,
     *                    values are assumed to be the same as the options.
     * @param $flags Flags for this attribute
     * @param $size  Size of the attribute.
     */
    function atkListAttribute($name, $optionArray, $valueArray="", $flags=0, $size=0)
    {
      if (!is_array($valueArray) || count($valueArray)==0)
      {
        $valueArray = $optionArray;
      }

      // If all values are numeric, we can use a numeric field to store the selected
      // value.
      $this->m_dbfieldtype = "number";
      for ($i=0, $_i=count($valueArray); $i<$_i && $this->m_dbfieldtype=="number"; $i++)
      {
        if (!is_numeric($valueArray[$i])) $this->m_dbfieldtype="string";
        // if one of the values is not a number, the fieldtype must be string, and
        // the loop is stopped.
      }

      // If no size is specified, the max size we have is equal to the biggest value.
      if ($size==0)
      {
        for ($i=0, $_i=count($valueArray); $i<$_i; $i++)
        {
          $size = max($size, strlen($valueArray[$i]));
        }
      }

      $this->atkAttribute($name, $flags, $size); // base class constructor
      $this->m_options = $optionArray;
      $this->m_values = $valueArray;

      for ($i=0, $_i=count($optionArray); $i<$_i; $i++)
      {
        $this->m_lookup[$this->m_values[$i]] = $optionArray[$i];
      }
    }


    /**
     * Display's text version of Record
     * @param $record
     * @return text string of $record
     */
    function display($record)
    {
      if(isset($this->m_lookup[$record[$this->fieldName()]]))
      {
        if($this->hasFlag(AF_NO_TRANSLATION)) $res = $this->m_lookup[$record[$this->fieldName()]];
        else $res = atktext($this->m_lookup[$record[$this->fieldName()]],$this->m_ownerInstance->m_module, $this->m_owner);

        return $res;
      }
    }

    /**
     * Returns a piece of html code that can be used in a form to edit this
     * attribute's value.
     * @param $record Array with fields
     * @return piece of html code with a checkbox
     */
    function edit($record="", $fieldprefix="")
    {
      // todo: configurable rows
      $id = $this->getHtmlId($fieldprefix);
      $this->registerKeyListener($id, KB_CTRLCURSOR|KB_LEFTRIGHT);

      $onchange='';
      if (count($this->m_onchangecode))
      {
        $onchange = 'onChange="'.$id.'_onChange(this);"';
        $this->renderChangeHandler($fieldprefix);
      }

      $result = '<select id="'.$id.'" name="'.$id.'"  class="atklistattribute" '.$onchange.($this->m_width?" style='width: {$this->m_width}px'":"").'>';

      if(!$this->hasFlag(AF_OBLIGATORY)&&!$this->hasFlag(AF_LIST_NO_NULL_ITEM)||(atkconfig("list_obligatory_null_item")&&$record[$this->fieldName()]==""&&!$this->hasFlag(AF_LIST_NO_OBLIGATORY_NULL_ITEM)))
      {
        $result.= '<option value="'.$this->m_emptyvalue.'">'.atktext('list_null_value', '', $this->m_ownerInstance->m_type);
      }
      for ($i=0;$i<count($this->m_values);$i++)
      {
        // If the current value is selected or occurs in the record
        if ((!is_null($this->m_selected) && $this->m_values[$i] == $this->m_selected) ||
            ( is_null($this->m_selected) && $this->m_values[$i] == $record[$this->fieldName()] && $record[$this->fieldName()]!=""))
        {
          $sel = "selected";
        }
        else
        {
          $sel = "";
        }
        if ($this->hasFlag(AF_NO_TRANSLATION)) $option=$this->m_options[$i];
        else $option = atktext($this->m_options[$i],$this->m_ownerInstance->m_module, $this->m_owner);

        $result.= '<option value="'.$this->m_values[$i].'" '.$sel.'>'.$option.'</option>';
      }
      $result.='</select>';
      return $result;
    }

    /**
     * Returns a piece of html code that can be used in a form to search values
     * @param $record Array with fields
     * @return piece of html code with a checkbox
     */
    function search($record="", $extended=false, $fieldprefix="")
    {
      // todo: configurable rows
      $result = '<select ';
      if ($extended)
      {
        $result.='multiple size="'.min(5,count($this->m_values)+1).'"';
      }

      if (strstr($fieldprefix,"]["))
        $result.='name="atksearch['. $fieldprefix.$this->fieldName().'][]">';
      else
        $result.='name="atksearch'.$fieldprefix.'['.$this->formName().'][]">';

      $result.= '<option value="">'.atktext('search_all');

      for ($i=0,$_i=count($this->m_values);$i<$_i;$i++)
      {
        if (atk_in_array(((string)$this->m_values[$i]),$record[$this->fieldName()], true) && $record[$this->fieldName()]!=="")
        {
          $sel = "selected";
        }
        else
        {
          $sel = "";
        }
        if($this->hasFlag(AF_NO_TRANSLATION)) $string = $this->m_options[$i];
        else $string = atktext($this->m_options[$i], $this->m_ownerInstance->m_module);

        $result.= '<option value="'.$this->m_values[$i].'" '.$sel.'>'.$string;
      }
      $result.='</select>';
      return $result;
    }

    /**
     * Creates an search condition for a given search value
     * @param $table the table name
     * @param $value the search value
     * @return a piece of the SQL where clause
     */
    function searchCondition(&$query, $table, $value, $searchmode)
    {
      $query->addSearchCondition($this->getSearchCondition($query, $table, $value, $searchmode));
    }

    function getSearchCondition(&$query, $table, $value, $searchmode)
    {
    	// We only support 'exact' matches.
      // But you can select more than one value, which we search using the IN() statement,
      // which should work in any ansi compatible database.
      if (is_array($value) && count($value)>0 && $value[0]!="") // This last condition is for when the user selected the 'search all' option, in which case, we don't add conditions at all.
      {
        if (count($value)==1) // exactly one value
        {
          $searchcondition = $query->exactCondition($table.".".$this->fieldName(),escapeSQL($value[0]));
        }
        else // search for more values using IN()
        {
          $searchcondition = $table.".".$this->fieldName()." IN ('".implode("','",$value)."')";
        }
      }
      return $searchcondition;
    }

    function getSearchModes()
    {
      // exact match and substring search should be supported by any database.
      // (the LIKE function is ANSI standard SQL, and both substring and wildcard
      // searches can be implemented using LIKE)
      // Possible values
      //"regexp","exact","substring", "wildcard","greaterthan","greaterthanequal","lessthan","lessthanequal"
      return array("exact");
    }

    function dbFieldType()
    {
      // Fieldtype was determined in the constructor.
      return $this->m_dbfieldtype;
    }

    function renderChangeHandler($fieldprefix)
    {
      if (count($this->m_onchangecode))
      {
        $page = &$this->m_ownerInstance->getPage();
        $page->register_scriptcode("function ".$this->getHtmlId($fieldprefix)."_onChange(el)
                                    {
                                      newvalue = el.options[el.selectedIndex].value;
                                      ".implode("\n", $this->m_onchangecode)."
                                    }\n");
      }
    }

    function setAutoHide($attrib, $valuearr)
    {
      $conditions = array();
      foreach ($valuearr as $value)
      {
        $conditions[] = "newvalue=='$value'";
      }
      $this->addOnChangeHandler("if (".implode('||', $conditions).") hideAttrib('$attrib'); else showAttrib('$attrib');");
    }

    /**
     * Sets the selected listitem
     * @param string $selected the listitem you want to have selected
     */
    function setSelected($selected)
    {
      $this->m_selected = $selected;
    }

    /**
     * Gets the selected listattribute
     * @return string the selected listitem
     */
    function getSelected()
    {
      return $this->m_selected;
    }

    /**
     * Sets the value for the empty entry in the list attribute
     * In normal cases you would just leave this empty, but certain cases
     * might demand you set a value.
     * @param string $value the value we set for empty value
     */
    function setEmptyValue($value)
    {
      $this->m_emptyvalue = $value;
    }

    /**
     * Gets the value for the empty entry in the list attribute
     * @return string
     */
    function getEmptyValue()
    {
      return $this->m_emptyvalue;
    }

    /**
     * Convert a String representation into an internal value.
     *
     * This implementation search for the value first in the valueArray, then in the optionArray
     * All other values are converted to the first of the valueArray
     *
     * @param String $stringvalue The value to parse.
     * @return Internal value (from valueArray)
     */
    function parseStringValue($stringvalue)
    {
      foreach($this->m_values as $value)
        if(strtolower($stringvalue) == strtolower($value))
          return $value;

      $i=0;
      foreach($this->m_options as $option)
      {
        if(strtolower($stringvalue) == strtolower($option))
          return $this->m_values[$i];

        if(strtolower(atktext($stringvalue)) == strtolower($option))
          return $this->m_values[$i];
        $i++;
      }

      return $this->m_value[0];
    }

    /**
     * Set the width of the dropdown list in pixels
     * @param int $width The width of the dropdown list in pixels
     */
    function setWidth($width)
    {
      $this->m_width=$width;
    }

    /**
     * Gets the width of the dropdown list in pixels
     * @return int The width of the dropdown list in pixels
     */
    function getWidth()
    {
      return $this->m_width;
    }
  }

?>