/*
 * ALSA ASoC Machine Driver for Allo Piano DAC
 *
 * Author:	Baswaraj K <jaikumar@cem-solutions.net>
 *		Copyright 2016
 *		based on code by Daniel Matuschek <info@crazy-audio.com>
 *		based on code by Florian Meier <florian.meier@koalo.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/module.h>
#include <linux/platform_device.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include "../codecs/pcm512x.h"

static bool digital_gain_0db_limit = true;

static int snd_allo_piano_dac_init(struct snd_soc_pcm_runtime *rtd)
{
	if (digital_gain_0db_limit) {
		int ret;
		struct snd_soc_card *card = rtd->card;

		ret = snd_soc_limit_volume(card, "Digital Playback Volume",
					   207);
		if (ret < 0)
			dev_warn(card->dev, "Failed to set volume limit: %d\n",
				 ret);
	}

	return 0;
}

static int snd_allo_piano_dac_hw_params(
	struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

	unsigned int sample_bits =
		snd_pcm_format_physical_width(params_format(params));
#if 0
	int ret = 0, val = 0;
	val = snd_soc_read(rtd->codec, PCM512x_RATE_DET_4);
	if (val < 0) {
		dev_err(rtd->codec->dev,
				"Failed to read register PCM512x_RATE_DET_4\n");
			return val;
	}

	if (val & 0x40) {
		ret = snd_soc_write(rtd->codec,
					PCM512x_PLL_REF,
					PCM512x_SREF_BCK);
		if (ret < 0)
			return ret;

		dev_info(rtd->codec->dev,
			"Setting BCLK as input clock and Enable PLL\n");
	} else {
		ret = snd_soc_write(rtd->codec,
					PCM512x_PLL_EN,
					0x00);
		if (ret < 0)
			return ret;

		ret = snd_soc_write(rtd->codec,
					PCM512x_PLL_REF,
					PCM512x_SREF_SCK);
		if (ret < 0)
			return ret;

		dev_info(rtd->codec->dev,
			"Setting SCLK as input clock and disabled PLL\n");
	}
#endif
	return snd_soc_dai_set_bclk_ratio(cpu_dai, sample_bits * 2);
}

/* machine stream operations */
static struct snd_soc_ops snd_allo_piano_dac_ops = {
	.hw_params = snd_allo_piano_dac_hw_params,
};

static struct snd_soc_dai_link snd_allo_piano_dac_dai[] = {
{
	.name		= "Piano DAC",
	.stream_name	= "Piano DAC HiFi",
	.cpu_dai_name	= "bcm2708-i2s.0",
	.codec_dai_name	= "pcm512x-hifi",
	.platform_name	= "bcm2708-i2s.0",
	.codec_name	= "pcm512x.1-004c",
	.dai_fmt	= SND_SOC_DAIFMT_I2S |
			  SND_SOC_DAIFMT_NB_NF |
			  SND_SOC_DAIFMT_CBS_CFS,
	.ops		= &snd_allo_piano_dac_ops,
	.init		= snd_allo_piano_dac_init,
},
};

/* audio machine driver */
static struct snd_soc_card snd_allo_piano_dac = {
	.name         = "PianoDAC",
	.owner        = THIS_MODULE,
	.dai_link     = snd_allo_piano_dac_dai,
	.num_links    = ARRAY_SIZE(snd_allo_piano_dac_dai),
};

static int snd_allo_piano_dac_probe(struct platform_device *pdev)
{
	int ret = 0;

	snd_allo_piano_dac.dev = &pdev->dev;

	if (pdev->dev.of_node) {
		struct device_node *i2s_node;
		struct snd_soc_dai_link *dai;

		dai = &snd_allo_piano_dac_dai[0];
		i2s_node = of_parse_phandle(pdev->dev.of_node,
					    "i2s-controller", 0);

		if (i2s_node) {
			dai->cpu_dai_name = NULL;
			dai->cpu_of_node = i2s_node;
			dai->platform_name = NULL;
			dai->platform_of_node = i2s_node;
		}

		digital_gain_0db_limit = !of_property_read_bool(
			pdev->dev.of_node, "allo,24db_digital_gain");
	}

	ret = snd_soc_register_card(&snd_allo_piano_dac);
	if (ret)
		dev_err(&pdev->dev,
			"snd_soc_register_card() failed: %d\n", ret);

	return ret;
}

static int snd_allo_piano_dac_remove(struct platform_device *pdev)
{
	return snd_soc_unregister_card(&snd_allo_piano_dac);
}

static const struct of_device_id snd_allo_piano_dac_of_match[] = {
	{ .compatible = "allo,piano-dac", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, snd_allo_piano_dac_of_match);

static struct platform_driver snd_allo_piano_dac_driver = {
	.driver = {
		.name   = "snd-allo-piano-dac",
		.owner  = THIS_MODULE,
		.of_match_table = snd_allo_piano_dac_of_match,
	},
	.probe          = snd_allo_piano_dac_probe,
	.remove         = snd_allo_piano_dac_remove,
};

module_platform_driver(snd_allo_piano_dac_driver);

MODULE_AUTHOR("Baswaraj K <jaikumar@cem-solutions.net>");
MODULE_DESCRIPTION("ALSA ASoC Machine Driver for Allo Piano DAC");
MODULE_LICENSE("GPL v2");
