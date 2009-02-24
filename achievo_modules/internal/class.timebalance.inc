<?php
/*
 * Copyright 2008 Develer S.r.l. (http://www.develer.com/)
 * All rights reserved.
 *
 * $Id:$
 * Author: Matteo Bertini <naufraghi@develer.com>
 */

/*
SELECT userid, firstname, lastname, activitydate, time_contract, SUM(time_worked) as time_worked,
    SUM(time_malattia) as time_malattia, SUM(time_ferie) as time_ferie, SUM(time_d20) as time_d20,
    SUM(tickets) as tickets
    FROM (
        SELECT userid, firstname, lastname, activitydate, time_contract,
            (NOT (malattia OR ferie OR d20)) * SUM(time_spent)/60 AS time_worked,
            malattia * SUM(time_spent)/60 AS time_malattia,
            ferie * SUM(time_spent)/60 AS time_ferie,
            d20 * SUM(time_spent)/60 AS time_d20,
            (NOT (malattia OR ferie OR d20)) * SUM(time_spent > 5*60) AS tickets
            FROM (
                SELECT person.userid, person.firstname, person.lastname,
                    usercontract.uc_hours/5 as time_contract, project.name AS project_name,
                    phase.name AS phase_name, hours.activitydate, sum(hours.time) AS time_spent,
                    (project.name = '__dipendenti' AND phase.name = 'malattia') AS malattia,
                    (project.name = '__dipendenti' AND phase.name = 'ferie') AS ferie,
                    (project.name LIKE 'd20_%') AS d20
                FROM hours
                    JOIN person ON hours.userid = person.id
                    JOIN phase ON hours.phaseid = phase.id
                    JOIN activity ON hours.activityid = activity.id
                    JOIN project ON phase.projectid = project.id
                    JOIN usercontract ON usercontract.userid = person.id
                    WHERE activitydate > '2008-08-05'
                GROUP BY person.firstname, person.lastname, person.userid, usercontract.uc_hours,
                         hours.activitydate, malattia, ferie, d20
            ) AS lavoro
        GROUP BY userid, firstname, lastname, activitydate, time_contract, malattia, ferie, d20
    ) AS parts
GROUP BY userid, firstname, lastname, activitydate, time_contract
*/

useattrib("atkdateattribute");
atkImport("modules.scheduler.scheduler_holidays");

function str_number_format($var, $digits)
{
    if (is_numeric($var))
        return number_format($var, $digits, ",", ".");
    else
        return $var;
}

//Supporto per la colorazione alternata delle righe
$_flipflop = true;
function flipflop($flip=true, $flop=false)
{
    global $_flipflop;
    $_flipflop = !$_flipflop;
    if ($_flipflop)
        return $flip;
    else
        return $flop;
}

function get_days_array($date_start, $date_end)
{
    $start_date = date_create($date_start);
    $stop_date = date_create($date_end);
    $curr_date = date_create($date_start);
    $days = array();
    while ($curr_date <= $stop_date)
    {
        array_push($days, date_create($curr_date->format(DATE_ATOM)));
        $curr_date->modify("+1 day");
    }
    return $days;
}

function f($float)
{
    return str_number_format($float, 2);
}

function format_date($date)
{
    return $date->format("Y-m-d");
}

class timebalance extends atkNode
{
    function timebalance()
    {
        $this->atkNode("timebalance");
        $this->overtime_node = &atkGetNode("timereg.overtime_balance");
        $this->holidays_node = &atkGetNode("scheduler.scheduler_holidays");
    }

    function get_userid() {
        if ( $this->allowed("any_user") && $this->m_postvars['userid'] ) {
            return $this->m_postvars['userid'];
        } else {
            global $g_user;
            return $g_user["id"];
        }
    }

    function get_timesummary()
    {

        $userid = $this->get_userid();
        $db = &atkGetDb();

        /* Accetta una data in ingresso oppure imposta l'intervallo all'ultimo mese appena concluso*/
        $last_month_start = mktime(0, 0, 0, date("m")-1, 1, date("Y"));
        $last_month_end = mktime(0, 0, 0, date("m"), 0, date("Y"));
        $date_start = $this->m_postvars['date_start'];
        $date_end = $this->m_postvars['date_end'];
        if ($date_start == "")
            $date_start = date("Y-m-d", $last_month_start);
        else
            $date_start = sprintf("%04d",$date_start["year"])."-".sprintf("%02d",$date_start["month"])."-".sprintf("%02d",$date_start["day"]);
        if ($date_end == "")
            $date_end = date("Y-m-d", $last_month_end);
        else
            $date_end = sprintf("%04d",$date_end["year"])."-".sprintf("%02d",$date_end["month"])."-".sprintf("%02d",$date_end["day"]);

        if ( $userid != 'all' )
            $single_user = "AND hours.userid ='$userid'";
        else
            $single_user = "";

        $ore_registrate = "
                SELECT person.userid, person.firstname, person.lastname,
                    project.name AS project_name,
                    phase.name AS phase_name, hours.activitydate, sum(hours.time) AS time_spent,
                    (project.name = '__dipendenti' AND phase.name = 'malattia') AS malattia,
                    (project.name = '__dipendenti' AND phase.name = 'ferie') AS ferie,
                    (project.name LIKE 'd20_%') AS d20
                FROM hours
                    JOIN person ON hours.userid = person.id
                    JOIN phase ON hours.phaseid = phase.id
                    JOIN activity ON hours.activityid = activity.id
                    JOIN project ON phase.projectid = project.id
                    JOIN usercontract ON usercontract.userid = person.id
                WHERE activitydate >= '$date_start'
                    AND activitydate <= '$date_end'
                    $single_user
                GROUP BY person.firstname, person.lastname, person.userid,
                    hours.activitydate, malattia, ferie, d20
        ";

        $select = "
SELECT userid, firstname, lastname, activitydate, SUM(time_worked) as time_worked,
    SUM(time_malattia) as time_malattia, SUM(time_ferie) as time_ferie, SUM(time_d20) as time_d20,
    SUM(tickets) as tickets
    FROM (
        SELECT userid, firstname, lastname, activitydate,
            (NOT (malattia OR ferie OR d20)) * SUM(time_spent)/60 AS time_worked,
            malattia * SUM(time_spent)/60 AS time_malattia,
            ferie * SUM(time_spent)/60 AS time_ferie,
            d20 * SUM(time_spent)/60 AS time_d20,
            (NOT (malattia OR ferie OR d20)) * SUM(time_spent >= 5*60) AS tickets
            FROM (
                $ore_registrate
            ) AS lavoro
        GROUP BY userid, firstname, lastname, activitydate, malattia, ferie, d20
    ) AS parts
GROUP BY userid, firstname, lastname, activitydate;
";
        return array("date_start" => $date_start,
                     "date_end" => $date_end,
                     "rows" => $db->getRows($select));
    }

    function action_report()
    {
        $userid = $this->get_userid();
        $ui = &$this->getUi();
        $page = &$this->getPage();
        $this->addStyle("style.css");
        $content = '<form action="dispatch.php" method="get" name="entryform">';
        $content .= session_form();

        $results = $this->get_timesummary();
        $title = "TimeBalance: ".$results["date_start"]." - ".$results["date_end"];

        $startdateatt = new atkDateAttribute("date_start", "F d Y", "d F Y",0 , 0, AF_OBLIGATORY);
        $enddateatt = new atkDateAttribute("date_end", "F d Y", "d F Y", 0, 0, AF_OBLIGATORY);

        $_startdate = array("date_start" => $startdateatt->dateArray($results["date_start"]));
        $_enddate = array("date_end" => $enddateatt->dateArray($results["date_end"]));

        $daykeys = array_map("format_date", get_days_array($results["date_start"], $results["date_end"]));
        $workdays = $this->overtime_node->enumWorkingDays($results["date_start"], $results["date_end"], $userid, 'Y-m-d');
        atkdebug("workdays: ".var_export($workdays, 1));
        $alldays = array();
        foreach ($daykeys as $day) {
            $alldays[$day] = array('time_contract' => $workdays[$day]);
        }

        $content .= "Da ".$startdateatt->edit($_startdate)." a ".$enddateatt->edit($_enddate)."<br>\n";
        $weekreport = &atkGetNode("reports.weekreport");
        $content .= 'User: <SELECT name="userid">'.$weekreport->get_employees($userid, $key="userid").'</SELECT>';
        $content .= '<input type="submit" value="'.atktext("refresh").'">';
        $content .= "<h2>$title</h2>\n";

        // Prepara dati in struttura 3d
        $time_balance = array();
        //$tot_contract = $this->overtime_node->getContractHours($results["date_start"], $results["date_end"], $userid);
        foreach ($results['rows'] as $key => $r)
        {
            if (!array_key_exists($r['userid'], $time_balance))
            {
                $time_balance[$r['userid']] = array('fullname' => $r['firstname']." ".$r['lastname'],
                                                    'activity' => $alldays,
                                                    'tot_contract' => array_sum($workdays),
                                                    'tot_registro' => 0.0,
                                                    'tot_worked' => 0.0,
                                                    'tot_malattia' => 0.0,
                                                    'tot_ferie' => 0.0,
                                                    'tot_d20' => 0.0,
                                                    'tot_tickets' => 0,
                                                   );
            }
            $time_balance[$r['userid']]['activity'][$r['activitydate']] = array('time_contract' => floatval($workdays[$r['activitydate']]),
                                                                                'time_worked' => floatval($r['time_worked']),
                                                                                'time_malattia' => floatval($r['time_malattia']),
                                                                                'time_ferie' => floatval($r['time_ferie']),
                                                                                'time_d20' => floatval($r['time_d20']),
                                                                                'tickets' => intval($r['tickets']));
            $time_balance[$r['userid']]['tot_worked'] += floatval($r['time_worked']);
            $time_balance[$r['userid']]['tot_malattia'] += floatval($r['time_malattia']);
            $time_balance[$r['userid']]['tot_ferie'] += floatval($r['time_ferie']);
            $time_balance[$r['userid']]['tot_d20'] += floatval($r['time_d20']);
            $time_balance[$r['userid']]['tot_tickets'] += intval($r['tickets']);
            atkdebug("TimeBalance loop: ".var_export($r, 1));
        }
        atkdebug("TimeBalance: ".var_export($time_balance, 1));
        // Tabelle banca ore
        $banca_head = "<tr><th>User</th><th>Nome</th><th>Giorno</th><th>Ore previste</th><th>Ore registro</th>
                           <th>Ore lavorate</th><th>Malattia</th><th>Ferie</th><th>Develer20</th><th>Tickets</th>
                           </tr>\n";
        foreach ($time_balance as $userid => $userdata)
        {
            $content .= "<h3>$userid - ".$userdata['fullname']."</h3>\n";
            $content .= "<table border='1'>\n";
            $content .= $banca_head;

            $balance = $this->overtime_node->getBalance($results["date_end"], $userid);
            atkDebug("balance = ".var_export($balance, 1));
            $banca_ore = $balance['balance'];

            foreach ($userdata['activity'] as $date => $activity)
            {
                $bgcolor = flipflop("#9999FF", "#BBBBFF");

                $abstime = strtotime($date);

                $content .= "<tr style='background: $bgcolor'>";
                $content .= "<td>$userid</td>";
                $content .= "<td>".$userdata['fullname']."</td>";
                if ( $this->holidays_node->isHoliday($date) ) {
                    $content .= "<td><b><i>".strftime("%Y-%m-%d %a", $abstime)."</i></b></td>";
                } elseif ( !array_key_exists($date, $workdays) ) {
                    $content .= "<td><b>".strftime("%Y-%m-%d %a", $abstime)."</b></td>";
                } else {
                    $content .= "<td>".strftime("%Y-%m-%d %a", $abstime)."</td>";
                }
                if ( array_key_exists($date, $workdays) ) {
                    $content .= "<td align='right'>".f($activity['time_contract'])."</td>";
                    $ore_registro = $activity['time_contract'] - $activity['time_malattia'] - $activity['time_ferie'];
                    $ore_registro = min($ore_registro, $banca_ore);
                    $banca_ore -= $ore_registro;
                    $content .= "<td align='right'>".f($ore_registro)."</td>";
                    $userdata['tot_registro'] += $ore_registro;
                } else {
                    $content .= "<td align='right'>&nbsp</td>"; // time_contract
                    $content .= "<td align='right'>&nbsp</td>"; // ore_registro
                }
                if ($activity)
                {
                    $content .= "<td align='right'>".f($activity['time_worked'])."</td>";
                    $content .= "<td align='right'>".f($activity['time_malattia'])."</td>";
                    $content .= "<td align='right'>".f($activity['time_ferie'])."</td>";
                    $content .= "<td align='right'>".f($activity['time_d20'])."</td>";
                    $content .= "<td align='right'>".$activity['tickets']."</td>";
                }
                else
                {
                    $content .= "<td align='right'>&nbsp</td>";
                    $content .= "<td align='right'>&nbsp</td>";
                    $content .= "<td align='right'>&nbsp</td>";
                    $content .= "<td align='right'>&nbsp</td>";
                    $content .= "<td align='right'>&nbsp</td>";
                }
                $content .= "</tr>\n";
            }
            $content .= $banca_head;
            $content .= "<tr><td>$userid</td><td>".$userdata['fullname']."</td><th>Totale</th>";
            $content .= "<th align='right'>".f($userdata['tot_contract'])."</th>";
            $content .= "<th align='right'>".f($userdata['tot_registro'])."</th>";
            $content .= "<th align='right' title='".f($userdata['tot_worked'] - $userdata['tot_registro'])."'>";
                $content .= f($userdata['tot_worked'])."</th>";
            $content .= "<th align='right'>".f($userdata['tot_malattia'])."</th>";
            $content .= "<th align='right'>".f($userdata['tot_ferie'])."</th>";
            $content .= "<th align='right' title='".f($userdata['tot_d20'])."'>";
                $content .= f(min($userdata['tot_d20'], $userdata['tot_worked']*0.2))."</th>";
            $content .= "<th align='right'>".$userdata['tot_tickets']."</th>";
            $content .= "</tr>\n";
            $content .= "<tr><td colspan='10'>$title<dl>\n";
            $content .= "<b>$userid:</b> ".$userdata['fullname']."<br>";
            $content .= "<b>Ore Previste:</b> ".f($userdata['tot_contract'])."<br>";
            $content .= "<b>Ore Registro:</b> ".f($userdata['tot_registro'])."<br>";
            $content .= "<b>Ore Lavorate:</b> <a title='".f($userdata['tot_worked'] - $userdata['tot_registro'])."'>";
                $content .= f($userdata['tot_worked'])."</a><br>";
            $content .= "<b>Malattia:</b> ".f($userdata['tot_malattia'])."<br>";
            $content .= "<b>Ferie:</b> ".f($userdata['tot_ferie'])."<br>";
            $content .= "<b>Develer20:</b> <a title='".f($userdata['tot_d20'])."'>";
                $content .= f(min($userdata['tot_d20'], $userdata['tot_worked']*0.2))."</a><br>";
            $content .= "<b>Tickets:</b> ".$userdata['tot_tickets']."<br>";
            
            $content .= "</dl></td></tr>\n";
            $content .= "</table>\n";
        }

        $content .= "<p>Pagina creata il ".date_create()->format("Y-m-d")."</p>\n";
        $content .= '<input type="submit" value="'.atktext("refresh").'">';
        $content .= '<input type="hidden" name="atknodetype" value="'.$this->atkNodeType().'">';
        $content .= '<input type="hidden" name="atkaction" value="report">';

        $content .= '</form>';
        $page->addContent($ui->renderBox(array("title"=>$title,"content"=>$content)));
    }
}
?>
