package cycinf.cvars.usedtype;

import general.util.Validator;

import java.text.MessageFormat;

import cycinf.util.*;

public class NamedAggregateTempArgList extends UsedTypeList {
	private final UsedTypeLayer context;
	public final UsedTypeLayer getContext() {
		return context;
	}
	
	private final Identifier name;
	public final Identifier getName() {
		return name;
	}

	public NamedAggregateTempArgList(UsedTypeLayer context, Identifier name) {
		this.context = Validator.nonnull(context, "context");
		this.name = Validator.nonnull(name, "name");
	}

	private static final String l1 = "_ADARG", l2 = "AD";

	@Override
	public final String fieldLocatorStringBase() {
		return getContext().toString() + l2 + getName().toString() + l1;
	}

	private static final MessageFormat prettyFormat = new MessageFormat(
		"template arg. #{0} to {1} in {2}");
	@Override
	public final String prettyStringForVariable(int index) {
		return prettyFormat.format(new Object[] {index + 1,
			getName().prettyString(),
			getContext().prettyString()});
	}

	public static final NamedAggregateTempArgList parseBase1(String x)
		throws ParseException {
		String x1 = ParsingUtils.chopSuffix(x, l1);
		if (x1 == null)
			return null;
		String x2 = ParsingUtils.chopIdentifierSuffix(x1);
		if (x2 == null)
			throw new ParseException("NamedAggregateTempArgList expecting the name of the aggregate: " + x);
		Identifier name = ParsingUtils.ival;
		String x3 = ParsingUtils.chopSuffix(x2, l2);
		if (x3 == null)
			throw new ParseException("NamedAggregateTempArgList expecting " + l2 + ": " + x);
		UsedTypeLayer context = UsedTypeLayer.parse(x3);
		return new NamedAggregateTempArgList(context, name);
	}
}
