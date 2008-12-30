<?php
/**
 * This file is part of the Achievo ATK distribution.
 * Detailed copyright and licensing information can be found
 * in the doc/COPYRIGHT and doc/LICENSE files which should be 
 * included in the distribution.
 *
 * @package atk
 * @subpackage meta.relations
 *
 * @copyright (c) 2005 Peter C. Verhage <peter@achievo.org>
 * @license http://www.achievo.org/atk/licensing ATK Open Source License
 *
 * @version $Revision: 1.12 $
 * $Id: class.atkmetamanytoonerelation.inc,v 1.12 2007/10/23 10:24:34 dennis Exp $
 */

/**
 * @internal Include the base class.
 */
userelation("atkmanytoonerelation");
  
/**
 * Implementation of many-to-one relationships for meta nodes.
 *
 * @author Peter C. Verhage <peter@achievo.org>
 * @package atk
 * @subpackage meta.relations
 */        
class atkMetaManyToOneRelation extends atkManyToOneRelation
{
  var $m_accessor;
  var $m_options;
  
  /**
   * Default constructor.    
   */
  function atkMetaManyToOneRelation($name, $accessor, $template, $options, $flags=0)
  {
    //we must use $options["source"] (if it present) as name for atkManyToOneRelation, because 
    //in case of multi referential key $name must be array with referential keys
    if(atk_in_array("source",$options) && is_array($options["source"]))
      $name = $options["source"];
      
    $this->atkManyToOneRelation($name, $accessor, $flags);
    $this->m_accessor = $accessor;
    $this->m_options = $options;
    
    $this->setDescriptorTemplate($template);

    if (isset($options["filter"])) 
      $this->setDestinationFilter($options["filter"]);
      
    if (isset($options['join_filter']))
      $this->setJoinFilter($options['join_filter']);
    
    $cols = array();
    if (array_key_exists('listcols', $options))  
      $cols = $options['listcols'];
    else if (array_key_exists('listcolumns', $options))
      $cols = $options['listcolumns'];
      
    foreach ($cols as $col)
      $this->addListColumn($col);
  }
  
  /**
   * Create the instance of the destination. 
   *
   * If succesful, the instance is stored in the m_destInstance member variable.
   * 
   * @return boolean true if succesful, false if something went wrong.
   */
  function createDestination()
  {
    if (is_object($this->m_destInstance)) return TRUE;
    
    $policy = &$this->m_ownerInstance->getMetaPolicy();

    $this->m_destination = $policy->findDestination($this->m_accessor, false);
    atkdebug("atkMetaManyToOneRelation: {$this->m_accessor} destination '{$this->m_destination}'");    
    
    if ($this->m_destination == NULL) 
    {
      atkerror("No destination found for atkMetaManyToOneRelation with accessor '{$this->m_accessor}'!");
      return FALSE;
    }
    else if (!parent::createDestination()) 
    {
      atkerror("Cannot instantiate destination '{$this->m_destination}' for atkMetaManyToOneRelation with accessor '{$this->m_accessor}'!");        
      return FALSE;
    }    
    
    return TRUE;
  }
}  
?>