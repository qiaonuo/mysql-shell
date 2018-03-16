//@ Check Instance Configuration must work without a session
||Can't connect to MySQL server on 'localhost'

//@ Check Instance Configuration should fail if there's no session nor parameters provided
||An open session is required to perform this operation.

//@ First Sandbox
||

//@ Check Instance Configuration ok with a session
||


//@# Error: user has no privileges to run the checkInstanceConfiguration command (BUG#26609909)
|ERROR: The account 'test_user'@'%' is missing privileges required to manage an InnoDB cluster:|
|Missing privileges on schema 'mysql': SELECT.|
|Missing privileges on schema 'mysql_innodb_cluster_metadata': SELECT.|
|Missing privileges on schema 'sys': SELECT.|
|Missing privileges on table 'performance_schema.replication_applier_configuration': SELECT.|
|Missing privileges on table 'performance_schema.replication_applier_status': SELECT.|
|Missing privileges on table 'performance_schema.replication_applier_status_by_coordinator': SELECT.|
|Missing privileges on table 'performance_schema.replication_applier_status_by_worker': SELECT.|
|Missing privileges on table 'performance_schema.replication_connection_configuration': SELECT.|
|Missing privileges on table 'performance_schema.replication_connection_status': SELECT.|
|Missing privileges on table 'performance_schema.replication_group_member_stats': SELECT.|
|Missing privileges on table 'performance_schema.replication_group_members': SELECT.|
|Missing privileges on table 'performance_schema.threads': SELECT.|
||Dba.checkInstanceConfiguration: The account 'test_user'@'%' is missing privileges required to manage an InnoDB cluster. (RuntimeError)


//@# Check instance configuration using a non existing user that authenticates as another user that does not have enough privileges (BUG#26979375)
|ERROR: The account 'test_user'@'%' is missing privileges required to manage an InnoDB cluster:|
|Missing privileges on schema 'mysql': SELECT.|
|Missing privileges on schema 'mysql_innodb_cluster_metadata': SELECT.|
|Missing privileges on schema 'sys': SELECT.|
|Missing privileges on table 'performance_schema.replication_applier_configuration': SELECT.|
|Missing privileges on table 'performance_schema.replication_applier_status': SELECT.|
|Missing privileges on table 'performance_schema.replication_applier_status_by_coordinator': SELECT.|
|Missing privileges on table 'performance_schema.replication_applier_status_by_worker': SELECT.|
|Missing privileges on table 'performance_schema.replication_connection_configuration': SELECT.|
|Missing privileges on table 'performance_schema.replication_connection_status': SELECT.|
|Missing privileges on table 'performance_schema.replication_group_member_stats': SELECT.|
|Missing privileges on table 'performance_schema.replication_group_members': SELECT.|
|Missing privileges on table 'performance_schema.threads': SELECT.|
||Dba.checkInstanceConfiguration: The account 'test_user'@'%' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@# Check instance configuration using a non existing user that authenticates as another user that has all privileges (BUG#26979375)
||

//@# Check if all missing privileges are reported for user with no privileges
|ERROR: The account 'no_privileges'@'%' is missing privileges required to manage an InnoDB cluster:|
|Missing global privileges: CREATE USER, FILE, GRANT OPTION, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, SHUTDOWN, SUPER.|
|Missing privileges on schema 'mysql': DELETE, INSERT, SELECT, UPDATE.|
|Missing privileges on schema 'mysql_innodb_cluster_metadata': ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SELECT, SHOW VIEW, TRIGGER, UPDATE.|
|Missing privileges on schema 'sys': SELECT.|
|Missing privileges on table 'performance_schema.replication_applier_configuration': SELECT.|
|Missing privileges on table 'performance_schema.replication_applier_status': SELECT.|
|Missing privileges on table 'performance_schema.replication_applier_status_by_coordinator': SELECT.|
|Missing privileges on table 'performance_schema.replication_applier_status_by_worker': SELECT.|
|Missing privileges on table 'performance_schema.replication_connection_configuration': SELECT.|
|Missing privileges on table 'performance_schema.replication_connection_status': SELECT.|
|Missing privileges on table 'performance_schema.replication_group_member_stats': SELECT.|
|Missing privileges on table 'performance_schema.replication_group_members': SELECT.|
|Missing privileges on table 'performance_schema.threads': SELECT.|
||Dba.checkInstanceConfiguration: The account 'no_privileges'@'%' is missing privileges required to manage an InnoDB cluster. (RuntimeError)
