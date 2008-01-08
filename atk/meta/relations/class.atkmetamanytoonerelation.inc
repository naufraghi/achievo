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
 * @version $Revision: 1.3 $
 * $Id: class.atkmetamanytoonerelation.inc,v 1.3 2006/01/09 08:58:58 peter Exp $
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
  function atkMetaManyToOneRelation($name, $accessor, $template, $options, $flags)
  {
    $this->atkManyToOneRelation($name, $accessor, $flags);
    $this->m_accessor = $accessor;
    $this->m_options = $options;
    
    $this->setDescriptorTemplate($template);
    $this->setDestinationFilter($options["filter"]);    
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
    if ($this->m_destInstance != "") return TRUE;
    
    $policy = &$this->m_ownerInstance->getMetaPolicy();

    $this->m_destination = $policy->findDestination($this);
    atkdebug("atkMetaManyToOneRelation: {$this->m_accessor} destination {$this->m_destination}");    
    if ($this->m_destination == NULL) return FALSE;
    return parent::createDestination();
  }
}  
?>