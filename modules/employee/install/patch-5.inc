<?php

  $setup->installNode("employee.functionlevel");
  $setup->addColumn("person", "level", "int(10)", false, 0);

?>
