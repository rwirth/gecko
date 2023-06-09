/*
Copyright 2011 Bastian Loeher, Roland Wirth

This file is part of GECKO.

GECKO is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

GECKO is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "dspcalfilterplugin.h"
#include "pluginmanager.h"
#include "pluginconnectorqueued.h"

#include <QGridLayout>
#include <QLabel>

static PluginRegistrar registrar ("dspcalfilter", DspCalFilterPlugin::create, AbstractPlugin::GroupDSP);

DspCalFilterPlugin::DspCalFilterPlugin(int _id, QString _name)
    : BasePlugin(_id, _name)
{
    createSettings(settingsLayout);

    addConnector(new PluginConnectorQVUint(this,ScopeCommon::in,"in"));
    addConnector(new PluginConnectorQVDouble(this,ScopeCommon::out,"calorimetry"));

    std::cout << "Instantiated DspTimeFilterPlugin" << std::endl;
}

void DspCalFilterPlugin::createSettings(QGridLayout * l)
{
    // Plugin specific code here

    QWidget* container = new QWidget();
    {
        QGridLayout* cl = new QGridLayout;

        QLabel* label = new QLabel(tr("This plugin does a box filter convolution of the input data."));
        QLabel* wlabel = new QLabel(tr("Width"));
        QLabel* slabel = new QLabel(tr("Shift"));
        QLabel* alabel = new QLabel(tr("Gain"));

        widthSpinner = new QSpinBox();
        shiftSpinner = new QSpinBox();
        gainSpinner = new QDoubleSpinBox();

        widthSpinner->setValue(conf.width);
        shiftSpinner->setValue(conf.shift);
        gainSpinner->setValue(conf.gain);

        gainSpinner->setMaximum(5.0);
        gainSpinner->setMinimum(-5.0);
        gainSpinner->setSingleStep(0.1);

        shiftSpinner->setMinimum(-10000);
        shiftSpinner->setMaximum(10000);

        connect(widthSpinner,SIGNAL(valueChanged(int)),this,SLOT(widthChanged()));
        connect(shiftSpinner,SIGNAL(valueChanged(int)),this,SLOT(shiftChanged()));
        connect(gainSpinner,SIGNAL(valueChanged(double)),this,SLOT(attenuationChanged()));

        cl->addWidget(label,0,0,1,2);
        cl->addWidget(wlabel,1,0,1,1);
        cl->addWidget(slabel,2,0,1,1);
        cl->addWidget(alabel,3,0,1,1);
        cl->addWidget(widthSpinner,1,1,1,1);
        cl->addWidget(shiftSpinner,2,1,1,1);
        cl->addWidget(gainSpinner,3,1,1,1);

        container->setLayout(cl);
    }

    // End

    l->addWidget(container,0,0,1,1);
}

void DspCalFilterPlugin::widthChanged()
{
    int newValue = widthSpinner->value();
    this->conf.width = newValue;
}

void DspCalFilterPlugin::shiftChanged()
{
    int newValue = shiftSpinner->value();
    this->conf.shift = newValue;
}

void DspCalFilterPlugin::attenuationChanged()
{
    int newValue = gainSpinner->value();
    this->conf.gain = newValue;
}

void DspCalFilterPlugin::applySettings(QSettings* settings)
{
    QString set;
    settings->beginGroup(getName());
        set = "width";   if(settings->contains(set)) conf.width = settings->value(set).toInt();
        set = "shift";   if(settings->contains(set)) conf.shift = settings->value(set).toInt();
        set = "gain";   if(settings->contains(set)) conf.gain = settings->value(set).toDouble();
    settings->endGroup();

    widthSpinner->setValue(conf.width);
    shiftSpinner->setValue(conf.shift);
    gainSpinner->setValue(conf.gain);
}

void DspCalFilterPlugin::saveSettings(QSettings* settings)
{
    if(settings == NULL)
    {
        std::cout << getName().toStdString() << ": no settings file" << std::endl;
        return;
    }
    else
    {
        std::cout << getName().toStdString() << " saving settings...";
        settings->beginGroup(getName());
            settings->setValue("width",conf.width);
            settings->setValue("shift",conf.shift);
            settings->setValue("gain",conf.gain);
        settings->endGroup();
        std::cout << " done" << std::endl;
    }
}

void DspCalFilterPlugin::userProcess()
{
    //std::cout << "DspCalFilterPlugin Processing" << std::endl;
    QVector<uint32_t> idata = inputs->first()->getData().value< QVector<uint32_t> > ();
    SamDSP dsp;

    // Convert to double
    QVector<double> outData (idata.size ());
    std::copy (idata.begin(), idata.end(), outData.begin ());

    //dsp.vectorToFile(outData,"/tmp/cal.dat");

    if(conf.width > 1)
    {
        dsp.fast_pad(outData,conf.width,0,outData[0]);
        dsp.fast_boxfilter(outData,conf.width);
        outData.resize(idata.size());
    }
    if(conf.shift != 0) dsp.fast_shift(outData,conf.shift);
    if(conf.gain != 1.0) dsp.fast_scale(outData,conf.gain);

    outputs->first()->setData(QVariant::fromValue (outData));
}

/*!
\page dspcalfilterplg Calorimetry Filter Plugin
\li <b>Plugin names:</b> \c dspcalfilterplugin
\li <b>Group:</b> DSP

\section pdesc Plugin Description
The calorimetry filter plugin effectively performs a running average on its input by convolving it with a box kernel.
The width of the box can be configured. Additionally, a gain may be applied to the output values and the output vector may be shifted.

The box kernel sums over \c width values right of the current position.
To accommodate for the box width the input is padded to the left by \c width using the first value of the input and the filtered data is cropped so input and output have the same length.

\section attrs Attributes
None

\section conf Configuration
\li \b Width: Width of the box
\li \b Shift: The result is shifted by this amount (positive values shift right)
\li \b Gain: Gain to be applied after filtering

\section inputs Input Connectors
\li \c in \c &lt;uint32_t>: Input signal

\section outputs Output Connectors
\li \c calorimetry \c &lt;double>: The filtered signal
*/
