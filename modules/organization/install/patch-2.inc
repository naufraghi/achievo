<?php
  $setup->addColumn("organization", "mail_address", "varchar(100)", true, "");
  $setup->addColumn("organization", "mail_zipcode", "varchar(20)", true, "");
  $setup->addColumn("organization", "mail_city", "varchar(100)", true, "");
  $setup->addColumn("organization", "mail_state", "varchar(100)", true, "");
  $setup->addColumn("organization", "mail_country", "varchar(100)", true, "");
  
  $g_db->query("UPDATE organization SET mail_address=address, 
                                        mail_zipcode=zipcode, 
                                        mail_city=city,
                                        mail_state=state,
                                        mail_country=country");
    
?>
