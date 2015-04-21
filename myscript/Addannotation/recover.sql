delete from summary_result
where id>3;
delete from data_anno
where id>3;
delete from anno_table
where id>3;
select setval('anno_table_seq',4);
select setval('data_anno_seq',4);
vacuum full;
