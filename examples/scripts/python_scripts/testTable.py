from bokeh.models.widgets import DataTable, DateFormatter, TableColumn
from bokeh.models import ColumnDataSource
from pandas import DataFrame
from bokeh.plotting import show
from bokeh.core.enums import Align
DF = DataFrame({'col1': [1,2],'col2': [3,4]})
Columns = [TableColumn(field=Ci, title=Ci) for Ci in DF.columns] # bokeh columns
data_table = DataTable(columns=Columns, source=ColumnDataSource(DF)) # bokeh table
data_table.index_position = None

show(data_table)
