// Assumptions: smart deployment rountines available
//@ Initialization
var mysql = require('mysql');

function kill_sandbox(port) {
	if (__sandbox_dir)
	  dba.killSandboxInstance(port, {sandboxDir:__sandbox_dir});
	else
	  dba.killSandboxInstance(port);
}

function start_sandbox(port) {
	if (__sandbox_dir)
	    dba.startSandboxInstance(port, {sandboxDir:__sandbox_dir});
	else
	    dba.startSandboxInstance(port);
}

function ensureSuperReadOnly(connection) {
	var tmpSession = mysql.getClassicSession(connection);
	var res = tmpSession.runSql('set global super_read_only=ON');
	tmpSession.close();
}

var connection1 = {scheme: 'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'};
var connection2 = {scheme: 'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'};
var connection3 = {scheme: 'mysql', host: localhost, port: __mysql_sandbox_port3, user: 'root', password: 'root'};

var deployed_here = reset_or_deploy_sandboxes();

ensureSuperReadOnly(connection1);
ensureSuperReadOnly(connection2);

//@<OUT> Configures the instance, answers 'yes' on the read only prompt
var res = dba.configureLocalInstance(connection1);

//@<OUT> Configures the instance, read only set, no prompt
var res = dba.configureLocalInstance(connection2, {clearReadOnly: true});

//@<OUT> Configures the instance, no prompt
var res = dba.configureLocalInstance(connection3);

//@<OUT> Creates Cluster succeeds, answers 'yes' on read only prompt
shell.connect(connection1);
var cluster = dba.createCluster('sample');

//@<OUT> Adds a read only instance
cluster.addInstance(connection2);
wait_slave_state(cluster, uri2, "ONLINE");

//@<OUT> Adds other instance
cluster.addInstance(connection3);
wait_slave_state(cluster, uri3, "ONLINE");

//@<OUT> Rejoins an instance
kill_sandbox(__mysql_sandbox_port3);
wait_slave_state(cluster, uri3, "(MISSING)");
start_sandbox(__mysql_sandbox_port3);

ensureSuperReadOnly(connection3);
cluster.rejoinInstance(connection3);

delete cluster;
session.close();

//@<OUT> Kill sandbox 2
kill_sandbox(__mysql_sandbox_port2);

//@<OUT> Kill sandbox 3
kill_sandbox(__mysql_sandbox_port3);

//@<OUT> Kill sandbox 1
kill_sandbox(__mysql_sandbox_port1);

//@<OUT> Start sandbox 1
start_sandbox(__mysql_sandbox_port1);

//@<OUT> Start sandbox 2
start_sandbox(__mysql_sandbox_port2);

//@<OUT> Start sandbox 3
start_sandbox(__mysql_sandbox_port3);

//@<OUT> Reboot the cluster
shell.connect(connection1);
session.runSql('set global super_read_only=ON');
var cluster = dba.rebootClusterFromCompleteOutage("sample");
wait_slave_state(cluster, uri2, "ONLINE");
wait_slave_state(cluster, uri3, "ONLINE");

if (deployed_here)
  cleanup_sandboxes(true);