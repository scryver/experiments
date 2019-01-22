struct Training
{
    u32 inputCount;
    f32 *inputs;
    
    u32 outputCount;
    f32 *outputs;
};

struct TrainingSet
{
    u32 count;
    Training *set;
};

internal void
shuffle(RandomSeriesPCG *random, u32 dataCount, Training *data)
{
    for (u32 x = 0; x < dataCount; ++x)
    {
        u32 indexB = random_choice(random, dataCount);
        Training t = data[x];
        data[x] = data[indexB];
        data[indexB] = t;
    }
}

internal TrainingSet
parse_training(char *dataPath)
{
     TrainingSet result = {};
    
     ApiFile training = read_entire_file(dataPath);
    i_expect(training.content.size > 0);
    
    u32 *counts = (u32 *)training.content.data;
    result.count = *counts++;
    u32 inpCount = *counts++;
    u32 outCount = *counts++;
    
    f32 *data = (f32 *)counts;
    
    result.set = allocate_array(Training, result.count);
    for (u32 train = 0; train < result.count; ++train)
    {
        Training *t = result.set + train;
        t->inputCount = inpCount;
        t->outputCount = outCount;
        t->inputs = data;
        data += inpCount;
        t->outputs = data;
        data += outCount;
    }
    
    return result;
}

internal void
save_training(TrainingSet training, char *savePath)
{
    i_expect(training.count);
    
    FILE *f = fopen(savePath, "wb");
    if (f)
    {
    fwrite(&training.count, 4, 1, f);
        fwrite(&training.set[0].inputCount, 4, 1, f);
        fwrite(&training.set[0].outputCount, 4, 1, f);
        
        for (u32 train = 0; train < training.count; ++train)
        {
            Training *t = training.set + train;
            
            fwrite(t->inputs, t->inputCount * sizeof(f32), 1, f);
            fwrite(t->outputs, t->outputCount * sizeof(f32), 1, f);
        }
        
        fclose(f);
    }
}
