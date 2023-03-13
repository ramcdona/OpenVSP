from bokeh.models.widgets import DataTable, DateFormatter, TableColumn
from bokeh.models import ColumnDataSource
from pandas import DataFrame
from bokeh.plotting import show
from bokeh.core.enums import Align
#DF = DataFrame([('Case #', "1"),('Analysis', "Sweep"),("Method","VLM"),("\\(\\alpha\\) (°)","-20.0 to 20.0, npts: 8")])#,"\\(\\beta\\) (°)":"0","Wake Iterations":const.m_MachVec[0],"Wake Iterations":const.m_WakeIterVec[0]})
Columns = [TableColumn(field="Case #", title="Case #")] # bokeh columns
data_table = DataTable(columns=Columns, src = ColumnDataSource({"Case #":1})) # bokeh table
data_table.index_position = None
show(data_table)
