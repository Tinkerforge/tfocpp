from typing import List

from statham.schema.constants import Maybe
from statham.schema.elements import Array, Integer, Object, String
from statham.schema.property import Property


class SampledValueItem(Object, additionalProperties=False):

    value: str = Property(String(), required=True)

    context: Maybe[str] = Property(String(enum=['Interruption.Begin', 'Interruption.End', 'Sample.Clock', 'Sample.Periodic', 'Transaction.Begin', 'Transaction.End', 'Trigger', 'Other']))

    format: Maybe[str] = Property(String(enum=['Raw', 'SignedData']))

    measurand: Maybe[str] = Property(String(enum=['Energy.Active.Export.Register', 'Energy.Active.Import.Register', 'Energy.Reactive.Export.Register', 'Energy.Reactive.Import.Register', 'Energy.Active.Export.Interval', 'Energy.Active.Import.Interval', 'Energy.Reactive.Export.Interval', 'Energy.Reactive.Import.Interval', 'Power.Active.Export', 'Power.Active.Import', 'Power.Offered', 'Power.Reactive.Export', 'Power.Reactive.Import', 'Power.Factor', 'Current.Import', 'Current.Export', 'Current.Offered', 'Voltage', 'Frequency', 'Temperature', 'SoC', 'RPM']))

    phase: Maybe[str] = Property(String(enum=['L1', 'L2', 'L3', 'N', 'L1-N', 'L2-N', 'L3-N', 'L1-L2', 'L2-L3', 'L3-L1']))

    location: Maybe[str] = Property(String(enum=['Cable', 'EV', 'Inlet', 'Outlet', 'Body']))

    unit: Maybe[str] = Property(String(enum=['Wh', 'kWh', 'varh', 'kvarh', 'W', 'kW', 'VA', 'kVA', 'var', 'kvar', 'A', 'V', 'K', 'Celsius', 'Fahrenheit', 'Percent']))


class MeterValueItem(Object, additionalProperties=False):

    timestamp: str = Property(String(format='date-time'), required=True)

    sampledValue: List[SampledValueItem] = Property(Array(SampledValueItem), required=True)


class MeterValuesRequest(Object, additionalProperties=False):

    connectorId: int = Property(Integer(minimum=0), required=True)

    transactionId: Maybe[int] = Property(Integer())

    meterValue: List[MeterValueItem] = Property(Array(MeterValueItem), required=True)
